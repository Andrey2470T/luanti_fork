set(ao_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/ClientActiveObject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ClientActiveObjectMgr.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/GenericCAO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SmokePuffCSO.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TestCAO.cpp
)

set(event_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/EventReceiver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/InputHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/JoystickController.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/KeyCache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/KeyPress.cpp
)

set(map_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/ClientMap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MapblockMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MeshCollector.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MeshGenerator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MeshGeneratorThread.cpp
)

set(media_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/ClientMedia.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/FileCache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ImageFilters.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ImageSource.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TexturePaths.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/TextureSource.cpp
)

set(pipeline_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/Anaglyph.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Core.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Factory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Interlaced.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Plain.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SecondStage.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SideBySide.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Stereo.cpp
)

set(player_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/Camera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/LocalPlayer.cpp
)

set(render_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/Clouds.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MeshOperations.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Particles.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/RenderingEngine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Sky.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Tile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/WieldMesh.cpp
)

set(shadows_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/DynamicShadows.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DynamicShadowsRender.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ShadowsScreenQuad.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ShadowsShaderCallbacks.cpp
)

set(sound_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/sound/sound.cpp)

if(USE_SOUND)
	list(APPEND sound_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/sound/al_extensions.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/al_helpers.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/ogg_file.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/playing_sound.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/proxy_sound_manager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/sound_data.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/sound_manager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/sound_openal.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/sound/sound_singleton.cpp
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
	${CMAKE_CURRENT_SOURCE_DIR}/fontengine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/game_formspec.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/gameui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/guiscalingfilter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/hud.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/minimap.cpp
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
	PARENT_SCOPE
)
