set(ao_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/ao/clientactiveobject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/clientactiveobjectmgr.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/genericcao.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/smokepuffcso.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/testcao.cpp
)

set(event_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/event/eventreceiver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/inputhandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/joystickcontroller.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/keycache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/keypress.cpp
)

set(map_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/map/clientmap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/mapblockmesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/meshcollector.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/meshgenerator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/meshgeneratorthread.cpp
)

set(media_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/media/clientmedia.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/filecache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/imagefilters.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/imagesource.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/shader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/texturepaths.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/texturesource.cpp
)

set(pipeline_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/anaglyph.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/core.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/factory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/interlaced.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/pipeline.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/plain.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/secondstage.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/sidebyside.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/stereo.cpp
)

set(player_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/player/playercamera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/player/localplayer.cpp
)

set(render_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/render/camera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/clouds.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/defaultVertexTypes.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/meshbuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/meshcreator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/meshoperations.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/particles.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/renderingengine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/sky.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/tile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/wieldmesh.cpp
)

set(shadows_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/dynamicshadows.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/dynamicshadowsrender.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/shadowsscreenquad.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/shadowsshadercallbacks.cpp
)

set(sound_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/sound/sound.cpp)

if(USE_SOUND)
	list(APPEND sound_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/sound/al_extensions.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/al_helpers.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/oggfile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/playingsound.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/proxysoundmanager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/sounddata.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/soundmanager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/soundopenal.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/soundsingleton.cpp
	)
	set(SOUND_INCLUDE_DIRS
		${OPENAL_INCLUDE_DIR}
		${VORBIS_INCLUDE_DIR}
		${OGG_INCLUDE_DIR}
		PARENT_SCOPE)
	set(SOUND_LIBRARIES
		${OPENAL_LIBRARY}
		${VORBIS_LIBRARIES}
		PARENT_SCOPE)
endif()

set(ui_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/ui/fontengine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/gameformspec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/gameui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/guiscalingfilter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/hud.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/minimap.cpp
)

set(client_SRCS
	${ao_SRCS}
	${event_SRCS}
	${map_SRCS}
	${media_SRCS}
	${pipeline_SRCS}
	${player_SRCS}
	${render_SRCS}
	${shadows_SRCS}
	${sound_SRCS}
	${ui_SRCS}
	${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/clientenvironment.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/clientlauncher.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/game.cpp
	PARENT_SCOPE
)
