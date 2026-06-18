#!/bin/bash -e

rootdir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../.."

upload_linux() {
	local arch=$1
	local tmpname="luantifork-linux-${arch}-arch"
	local archname="luantifork-linux-${arch}.tar.gz"
	local tmppath="${rootdir}/${tmpname}"
	mkdir -p $tmppath

	cp -r "${rootdir}/bin" "${tmppath}/"
	cp -r "${rootdir}/builtin" "${tmppath}/"
	cp -r "${rootdir}/client" "${tmppath}/"
	cp -r "${rootdir}/clientmods" "${tmppath}/"
	cp -r "${rootdir}/fonts" "${tmppath}/"
	cp -r "${rootdir}/games" "${tmppath}/"
	cp -r "${rootdir}/lib" "${tmppath}/"
	cp -r "${rootdir}/locale" "${tmppath}/"
	cp -r "${rootdir}/misc" "${tmppath}/"
	cp -r "${rootdir}/mods" "${tmppath}/"
	cp -r "${rootdir}/po" "${tmppath}/"
	cp -r "${rootdir}/textures" "${tmppath}/"
	cp -r "${rootdir}/worlds" "${tmppath}/"

	tar -czf $archname $tmppath
	rm -rf $tmppath
}

upload_linux_server() {
	local tmpname="luantifork-linux-server-arch"
	local archname="luantifork-linux-server.tar.gz"
	local tmppath="${rootdir}/${tmpname}"
	mkdir -p $tmppath

	cp -r "${rootdir}/bin" "${tmppath}/"
	cp -r "${rootdir}/builtin" "${tmppath}/"
	cp -r "${rootdir}/games" "${tmppath}/"
	cp -r "${rootdir}/lib" "${tmppath}/"
	cp -r "${rootdir}/mods" "${tmppath}/"
	cp -r "${rootdir}/worlds" "${tmppath}/"

	tar -czf $archname $tmppath
	rm -rf $tmppath
}

upload_macos() {
	local app_path="${rootdir}/build/macos/luanti.app"
	local dmgname="luantifork-macos.dmg"

	hdiutil create -volname "Luanti" \
		-srcfolder "$app_path" \
		-ov \
		-format UDZO \
		"${rootdir}/${dmgname}" 2>/dev/null
}
