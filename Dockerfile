ARG DOCKER_IMAGE=alpine:3.16
FROM $DOCKER_IMAGE AS dev

ENV IRRLICHT_VERSION master
ENV SPATIALINDEX_VERSION 1.9.3
ENV LUAJIT_VERSION v2.1

RUN apk add --no-cache git build-base cmake curl-dev zlib-dev zstd-dev \
		sqlite-dev postgresql-dev hiredis-dev leveldb-dev sdl2-dev \
		gmp-dev jsoncpp-dev ninja ca-certificates

WORKDIR /usr/src/
RUN git clone --recursive https://github.com/jupp0r/prometheus-cpp/ && \
		cd prometheus-cpp && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local \
			-DCMAKE_BUILD_TYPE=Release \
			-DENABLE_TESTING=0 \
			-GNinja && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://github.com/libspatialindex/libspatialindex -b ${SPATIALINDEX_VERSION} && \
		cd libspatialindex && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://luajit.org/git/luajit.git -b ${LUAJIT_VERSION} && \
		cd luajit && \
		make && make install && \
	cd /usr/src/ && \

FROM dev as builder

COPY .git /usr/src/luanti_fork/.git
COPY CMakeLists.txt /usr/src/luanti_fork/CMakeLists.txt
COPY README.md /usr/src/luanti_fork/README.md
COPY minetest.conf.example /usr/src/luanti_fork/minetest.conf.example
COPY builtin /usr/src/luanti_fork/builtin
COPY cmake /usr/src/luanti_fork/cmake
COPY doc /usr/src/luanti_fork/doc
COPY fonts /usr/src/luanti_fork/fonts
COPY lib /usr/src/luanti_fork/lib
COPY misc /usr/src/luanti_fork/misc
COPY po /usr/src/luanti_fork/po
COPY src /usr/src/luanti_fork/src
COPY textures /usr/src/luanti_fork/textures

WORKDIR /usr/src/luanti_fork
RUN cmake -B build \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SERVER=TRUE \
		-DENABLE_PROMETHEUS=TRUE \
		-DBUILD_UNITTESTS=FALSE \
		-DBUILD_CLIENT=FALSE \
		-GNinja && \
	cmake --build build && \
	cmake --install build

ARG DOCKER_IMAGE=alpine:3.16
FROM $DOCKER_IMAGE AS runtime

RUN apk add --no-cache curl gmp libstdc++ libgcc libpq jsoncpp zstd-libs \
				sqlite-libs postgresql hiredis leveldb sdl2 && \
	adduser -D luanti_fork --uid 30000 -h /var/lib/luanti_fork && \
	chown -R luanti_fork:luanti_fork /var/lib/luanti_fork

WORKDIR /var/lib/luanti_fork

COPY --from=builder /usr/local/share/luanti_fork /usr/local/share/luanti_fork
COPY --from=builder /usr/local/bin/minetestserver /usr/local/bin/minetestserver
COPY --from=builder /usr/local/share/doc/luanti_fork/minetest.conf.example /etc/luanti_fork/minetest.conf
COPY --from=builder /usr/local/lib/libspatialindex* /usr/local/lib/
COPY --from=builder /usr/local/lib/libluajit* /usr/local/lib/
USER luanti_fork:luanti_fork

EXPOSE 30000/udp 30000/tcp
VOLUME /var/lib/luanti_fork/ /etc/luanti_fork/

ENTRYPOINT ["/usr/local/bin/luanti_forkserver"]
CMD ["--config", "/etc/luanti_fork/minetest.conf"]
