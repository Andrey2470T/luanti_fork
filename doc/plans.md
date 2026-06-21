`Планы в IrrlichtRedo`
----------------------

* Заменить меш загрузчики на Assimp

`Планы в Luanti Fork`
---------------------

* Расширение GFX (CSM графический API):
    * Добавить поддержку материалов для ItemDef и HudDef
    * (?) Удалить `vertex_type` параметр из `materials` таблицы
    * Поддержка кастомных рендер проходов и постэффектов
        * Три типа:
          * `custom`: кастомный проход делающий ручную настройку контекста и рендер через коллбэк в `gfx.add_custom_step`
          * `postprocess`: проход рендера в пространстве экрана, принимает только фрагментный шейдер
          * `builtin`: один из встроенных проходов в движке (draw3D, drawHud, postprocess, ...)
    * Добавить новые функции:
        * `gfx.get_render_step_chain()`
          * Returns the ordered table of all current render steps chain of the pipeline in the format:

            ```lua
            {
                {   -- [n] where 'n' is the order number
                    type = "custom"/"postprocess"/"builtin",
                    name = <string>, -- name for the step (for "builtin" type these are "draw3D", "drawHud"...)
                    enable = true/false, -- enable or disable the given step?
                },
                ...
            }
            ```

        * `gfx.set_render_step_order(name, order)`
          * Sets the order number for "name" step
          * If there is already some step with the given order, shift that and following steps forward by the unit in the render step chain
        * `gfx.enable_render_step(name, bool)`
          * Enable or disable the given render step
          * Each builtin and custom steps are enabled by default
        * `gfx.add_texture_buffer(name, list)`
          * Already present. Support `settings` field:

            ```lua
            {
                settings = {
                    -- Wrapping: "repeat", "clamp_to_edge", "clamp_to_border", "mirrored_repeat", "mirror_clamp_to_edge"
                    wrap_u = <Wrapping>,
                    wrap_v = <Wrapping>,
                    wrap_w = <Wrapping>,
                    -- Minimal filters: "nearest", "linear", "nearest_mipmap_nearest", "linear_mipmap_nearest", "nearest_mipmap_linear", "linear_mipmap_linear"
                    min_filter = <Minimal filters>,
                    -- Magnification filters: "nearest", "linear"
                    mag_filter = <Magnification filters>,
                    generate_mipmaps = true/false,
                    max_mip_level = <integer>
                }
            }
            ```

        * `gfx.add_custom_step(name, function(context))`
          * Adds "custom" type step doing the manual render configuration (transforms, viewports, materials, draw calls...)
          * `context` is the userdata accessing the render context

        * `gfx.get_posteffects()`
          * Returns the consequent list of all screen effects (bloom, distance-of-field, reflections and etc):

          ```lua
          {
              {
                  name = <string>,
                  shader = {
                      src = "bloom.glsl"/"bloom.fsh"/"<код>",
                      constants = {
                          {"BLOOM_RADIUS", "4"},
                          {"BLOOM_STRENGTH", "1.5"},
                          ...
                      },
                      includes = {"filters"},
                      on_set_uniforms = function(setter)
                      {
                          -- Set uniforms using the uniform setter
                          local pos = core.localplayer:get_pos()
                          setter.set("blurWeights", {0.05, 0.1, 0.3, 0.7, 1.0})
                      }
                  },
                  texture_buffer = <name>,
                  input = {"3d_render", "bloom_mask"},
                  -- Map the textures names to the cubmeap faces (if they are cubemaps)
                  output = {{"extract_bloom", 0}},
                  viewport = {x1=..., y1=..., x2=..., y2=...},
                  cliprect = {x1=..., y1=..., x2=..., y2=...},
                  blend = {<Blend table>},
                  line_width = <number>
              }
          }
          ```

        * `gfx.add_posteffect(<Posteffect table>)` -- adds a new post effect
        * `gfx.override_builtin_posteffect(<Posteffect table>)` -- overrides the builtin effects like bloom, godrays, fsaa and etc

    * Прямой доступ к рендер контексту (пропускается как `context` параметр в коллбэк `gfx.add_custom_step`):
         * `context:set_viewport({x1=..., y1=..., x2=..., y2=...})`
           * Sets the current viewport rectangle
         * `context:set_scissor(enable, <Rect>)`
           * Enables/disables the scissortest and sets the rectangle
         * `context:set_material(<Material table>)`
           * Sets the current material used by the following drawer calls
         * `context:set_transform(<Transform matrices>)`
           * Sets the current `world`, `view` and `projection` matrices
             used by the following drawer calls
         * `context:clear(clear_flag, clear_color, clear_depth)`
           * Clears the color, depth and stencil buffers
           * `clear_flag`: 0 (none), 1 (color), 2 (depth), 4 (stencil), 7 (all)
           * `clear_color`: ColorSpec
           * `clear_depth`: float
         * `context:draw_list(<Params table>)`
           * Params table:

             ```lua
             {
                primitive_type = "points"/"lines"/"line_strips"/"triangles"/"triangle_fans",
                vertices = {
                   {
                      pos = <float>,
                      color = <ColorSpec>,
                      normal = <vector3d>,
                      uv = <vector2d>
                   },
                   ...
                },
                indices = {<vertices indices list>}
             }
             ```

         * `context:draw_rect(<Params table>)`
           * Params table:

             ```lua
             {
                rect = {x1 = ..., y1 = ..., x2 = ..., y2 = ...},
                color = <ColorSpec or list of ColorSpec>,
             }
             ```

         * `context:draw_image(<Params table>)` -- requires the Image API
           * Params table:

             ```lua
             {
                image = <Image object>,
                rect = {x1 = ..., y1 = ..., x2 = ..., y2 = ...},
                color = <ColorSpec or list of ColorSpec>,
                uvs = {x1 = ..., y1 = ..., x2 = ..., y2 = ...}
             }
             ```

         * `context:draw_mesh(<Mesh table>)`:
           * Mesh table:

             ```lua
             {
                name = "*.obj"/"*.b3d"/"*.gltf"/"*.x",
             	  vertex_type = "standard3d"/"standard2d",
             	  use_aux1_attrib = <bool>,
             	  use_aux2_attrib = <bool>,
             	  triangulate = <bool>,
             	  merge_vertices = <bool>,
             	  generate_normals = <bool>,
             	  flip_normals = <bool>,
             	  limit_bone_weights = <bool>,
             	  flip_winding_order = <bool>
             }
             ```
    * Возможность настраивать загрузку моделей через Mesh table
        * Будет пропускаться в `context.draw_mesh(self, <Mesh table>)`, в поле `mesh` NodeDef, EntityDef

* Адаптировать поддержку атласов из old ветки
    * Вернуть отрисовку пиксельных рамок вокруг тайлов (для решения проблем фильтрации)
	* Добавить предзагрузку тайлов, которые будут упакованы в атлас, через atlas.json файл в модах

* Рендерить отдельную копию блока с крэками через режим overlay (#15287)

* Адаптировать hardware скининг из old ветки. Нереализованные изменения:
   1. Lua API для скелета и анимаций сущностей
   2. Другие виды интерполяций кадров (constant, sine, cubic)

* Добавить поддержку цветного освещения нод:
    * Клиентская карта цветов света блоков каждого чанка (ClientMapBlock)
    * Запекание цветов света в два последних канала "inAux" аттрибута

* Прямая работа с изображениями в модах (замена текстурных модификаторов)
	* Прямой доступ к пикселям и их модификация
	* Различные режимы смешивания (Normal, Alpha, Add, Multiply, Overlay, Screen и тд)
	* Различные операции (копирование, обрезка, поворот, масштабирование с фильтрами)
	* Image userdata:

	  ```lua
	  local stone_img = core.image("default_stone.png")

	  -- Gets the average stone_img brightness
	  local brightness = stone_img:get_brightness()
	  core.log("info", "Average image brightness: " .. tonumber(brightness))

	  -- Creates the horizontal gradient from fully red to fully image color
	  stone_img:set_mode("alpha")

	  local color = {r=255, g=0, b=0, a=255}
	  local w, h = stone_img:width(), stone_img:height()
	  for y = 1, h do
		for x = 1, w do
			stone_img:set_pixel(x, y, color)
			color.a = 255 - x
		end
		color.a = 255
	  end

	  ...

	  -- Inside `core.register_node` table:
	  tiles = {stone_img, stone_img, stone_img, stone_img, stone_img, stone_img}
	  ```

* Расширить звуковой API
	* Добавить больше параметров в Sound parameters table:

	  ```lua
	  {
		end_time = <number>,
		-- Ends playing the sound on `number` time point
		-- Cam be negative like `start_time`
		type = "sound"/"stream",
		-- Defines that how the sound data will be loaded in memory
		-- type=`sound`:
		-- fully loads the data from the source
		-- fits for short soundtracks like sound effects (door opening, walking, cracking and etc)
		-- this type is default
		-- type=`stream`:
		-- not fully loads the data from the source, the sound is loading on-the-fly and not caching anywhere
		-- fits for large audio files like music
		rollof_factor = <number>, -- fade speed
		doppler_factor = <number> -- Doppler's effect
	  }
	  ```

	* Использовать методы в sound handle:
		* `handle:stop()`: stops the sound, equivalent to `core.sound_stop(handle)`
		* `handle:pause()`: pauses the sound, it saves the current time position
		* `handle:resume()`: resumes the sound, should be called after `handle:pause()`
		* `handle:get_sound_info()`: returns the table with following fields:
		  ```lua
		  {
		  	gain = <number>,
		  	pitch = <number>,
		  	fade = <number>,
		  	position = <vector3d>, -- where the source is currently locating
		  	is_playing = true/false, -- this sound is currently playing?
		  	time = <number>, -- the current time point in the playing sound in seconds
		  	duration = <number>, -- the sound duration in seconds
		  }
		  ```

		* `handle:is_playing()`: returns if the sound is currently playing
		* `handle:set_gain(<number>)`
		* `handle:set_pitch(<number>)`
		* `handle:set_fade(<number>)`

* Поддержка кастомизации набора состояний нод (paramtype2="custom")
     * Количество состояний нод будет определяться битовыми полями
     * Возможность менять в состояниях поля "drawtype", "textures", "use_texture_alpha", ...
     * Пример регистрации дверной ноды с 32 "facedir", 4 "color" и 2 значениями ("door_open", "door_closed") в NodeDef таблице:

        ```lua
        {
           -- @bits: count of bits, must be not greater 8 (the param2 size is one-byte)
           -- @variants: descriptions of variants, it is possible to code 2^bits within the given bitfield
           paramtype2 = "custom",
           param2_states = {
              {
                 bits = 5,
                 variants = "facedir"             -- 32 variants
              },
              {
                 bits = 2,
                 variants = "color"                -- 4 variants
              },
              {
                 bits = 1,
                 variants = {
                    ["door_open"] = {              -- variant 0
                       drawtype = "nodebox",
                       tiles = {"mydoors:door_open.png"},
                       nodebox = {
                          type = "fixed",
                          fixed = {-0.5, -0.5, -0.5, 0.5, 1.5, -0.3}
                       }
                    },
                    ["door_closed"] = {           -- variant 1
                       drawtype = "nodebox",
                       tiles = {"mydoors:door_closed.png"},
                       nodebox = {
                          type = "fixed",
                          fixed = {-0.5, -0.5, -0.5, -0.3, 1.5, 0.5}
                       }
                    }
                 }
              }
           }
        }
        ```

* Добавить в методах ObjectRef возможность изменения позиции/скорости/поворота/масштаба с интерполяциями

* Поддержка отображения HUD мешей и их кастомизация

* Dual wielding (#14427)

* Перенести вещи, которые можно реализовать на Lua, из C++ части в builtin (hud, хотбар, полосы здоровья/дыхания, wiledmesh, система крафта и тд)

* Более расширенный API контроля над камерой игрока и возможность создавать вторичные камеры (Camera API #14325)

* Улучшенный контроль освещения блоков:

  ```lua
  { -- inside Node Definition table
    sunlight_propagates = false,
    -- If true, sunlight will go infinitely through this node
    sunlight_propagation = {
        axles = {"x", "-y", ...}, -- along which axles the light can propagate through the node?
        fade = 2 -- how many light points the light gets faded through the node?
        has_ao = true/false -- the node has the ambient occlusion?
	}
  }
  ```

* Улучшить рендеринг некоторых drawtype в жидкостях
	* Не рендерить грани жидкости вокруг погруженной в нее ноды, если нода на глубине больше 1 ноды

* Сделать интерполированное освещение сущностей вместо одного цвета света на модель

* Мержить Formspec/HUD Replacement (#14263)

* Различные баги:
   1. visual_size прикрепляемой сущности умножается на visual_size родителя.
   2. Туловище (base) прикрепленного игрока при установке смещения через player:set_attach остается на месте, а смещается только голова с камерой (#10101)
   3. Unload сущности игнорирует аттачменты (#14583)
   4. Тени от аттачментов не движутся с их родительскими сущностями (#12630)
   5. Игрок движется по инерции при зажатых клавишах перемещения если был вызван player:set_physics_override({speed=0}) (#10037)
   6. Рэйкасты работают неправильно на прикрепленных сущностях (#10304)
   7. Не работает смешивание анимаций (переход от одной анимаций к другой) (#14817)
   8. Сущности позади или внутри блоков могут быть указаны игроком (#15599)
   9. После unload сущностей с прикрепленными looped звуками звуки начинают играть везде
   10. Звук не останавливается после уничтожения сущности к которой он был прикреплен
