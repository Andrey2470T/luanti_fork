set(ao_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/ao/ClientActiveObject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/ClientActiveObjectMgr.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/GenericCAO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/SmokePuffCSO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ao/TestCAO.cpp
)

set(event_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/event/EventReceiver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/InputHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/JoystickController.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/KeyCache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/KeyPress.cpp
)

set(map_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/map/ClientMap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/MapblockMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/MeshCollector.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/MeshGenerator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/map/MeshGeneratorThread.cpp
)

set(media_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/media/ClientMedia.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/FileCache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/ImageFilters.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/ImageSource.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/Shader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/TexturePaths.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/media/TextureSource.cpp
)

set(pipeline_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/Anaglyph.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/Core.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/Factory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/Interlaced.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/Pipeline.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/Plain.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/SecondStage.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/SideBySide.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pipeline/Stereo.cpp
)

set(player_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/player/Camera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/player/LocalPlayer.cpp
)

set(render_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/render/Clouds.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/MeshOperations.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/Particles.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/RenderingEngine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/Sky.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/Tile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/WieldMesh.cpp
)

set(shadows_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/DynamicShadows.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/DynamicShadowsRender.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/ShadowsScreenQuad.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/shadows/ShadowsShaderCallbacks.cpp
)

set(sound_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/sound/Sound.cpp)

if(USE_SOUND)
	list(APPEND sound_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/sound/ALExtensions.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/ALHelpers.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/OggFile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/PlayingSound.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/ProxySoundManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/SoundData.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/SoundManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/SoundOpenAL.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/SoundSingleton.cpp
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
	${CMAKE_CURRENT_SOURCE_DIR}/ui/FontEngine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/GameFormspec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/GameUI.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/GUIScalingFilter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/Hud.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/Minimap.cpp
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
	${CMAKE_CURRENT_SOURCE_DIR}/Client.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ClientEnvironment.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ClientLauncher.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Game.cpp
	PARENT_SCOPE
)
