`Планы в IrrlichtRedo`
----------------------

* Рефакторить Video классы (добавить на замену новые классы из old ветки: DrawContext, ITexture, Texture2D, TextureCubeMap, VAO...)
* Перенести /src/GUI в luanti_fork/src/gui, объединить классы-дупликаты
* Удалить /src/IO, использовать cpp filesystem
* Заменить меш загрузчики на Assimp
* Удалить абстрактные классы из /include, объединив их с классами реализаций
* Распределить /include файлы по папкам

`Планы в Luanti Fork`
---------------------

* Адаптировать SSCSM выполнение от luk3yx: https://gitlab.com/luk3yx/minetest-sscsm

* Добавить API матриц и кватернионов в builtin (#8515)

* Добавить графический CSM API:
   * Поддержка материалов через таблицы.
      * Возможность задавать базовые параметры (глубина, смешивание, трафарет, шейдеры и тд) и PBR (roughness, metallic, specular, AO, emission и тд):

         ```lua
         materials = {
            {  -- первый слот
                blend = {
                    enable = true/false,
                    mode = "normal"/"alpha"/"add"/"subtract",
                    -- Blend functions: "zero", "one", "src_alpha", "one_minus_src_alpha", "dst_alpha", "one_minus_dst_alpha"...
                    srcrgb_func = <Blend functions>,
                    srca_func = <Blend functions>,
                    dstrgb_func = <Blend functions>,
                    desta_func = <Blend functions>,
                    color = <ColorSpec>
                },
                depth = {
                    enable = true/false,
                    -- Compare functions: "less", "equal", "greater"...
                    func = = <Compare functions>
                },
                stencil = {
                    enable = true/false,
                    func = = <Compare functions>,
                    ref = 1.0,
                    mode = "keep"/"zero"/"replace"...
                },
                cull = {
                    enable = true/false,
                    mode = "back"/"front"/"front_and_back"
                },
                shader = <Shader table>,
                texture_outputs = { -- map the texture buffer indices to the correponding target
                    texture_buffer = <name>, -- name of used texture buffer
                    index = <integer>,
                    cubemap_face = "posx"/"posy"/"posz"/"negx"/"negy"/"negz", -- for type="cubemap"
                    array_layer = <integer> -- for type="array"
                },
                viewport = {x1 = 0, y1 = 0, x2 = 1, y2 = 1},
                cliprect = {x1 = 0, y1 = 0, x2 = 1, y2 = 1},
                line_width = 2.0,
                pbr = {
                    color = <ColorSpec>,
                    metallic = <number>/<mapname>,
                    roughness = <number>/<mapname>,
                    ambient_occlusion = <number>/<mapname>,
                    normal = <mapname>,
                    emission = <number>/<mapname>,
                    transmission = <number>,
                    ior = <number>
                },
                on_set_uniforms = function(self, setter)
                {
                    local pos = core.localplayer:get_pos()
                    setter.set("cameraPos", {x=pos.x, y+pos.y+1.5, z=pos.z})
                    setter.set("animationTimer", core.get_us_time())
                }
            },
            ...
         }
         ```

      * Таблица `materials` будет доступна в NodeDef, ItemDef, EntityDef, ParticleSpawnerDef, HudDef
      * Подтаблица `shader`, задающая вершинный и фрагментный шейдеры:

         ```lua
         shader = {
            vertex = "block_vertex.glsl"/"block.vsh"/"<код>",
            geometry = "block_geometry.glsl"/"block.gsh"/"<код>",
            fragment = "block_fragment.glsl"/"block.fsh"/"<код>",
            constants = {
                {"ENABLE_NODE_SPECULAR", "1"},
                {"USE_DISCARD", "0"},
                ...
            },
            uniforms = {
                {name = "cameraPos", type = "vector3d", def_value = vector.new()},
                {name = "animationTimer", type = "float", def_value = 0.0},
                ...
            }
         }
         ```

      * Коллбэк таблицы (`on_set_uniforms = function(self, setter)`) дает возможность обновлять юниформы через сеттер каждый кадр
   * Поддержка кастомных шейдеров (через `materials` и `pipeline`)
   * Поддержка кастомных рендер проходов и постэффектов (`pipeline` таблица)
        * Три типа:
          * `3d`: кастомный проход для рендера 3d объектов с таблицей `materials`
          * `postprocess`: проход рендера в пространстве экрана, принимает только фрагментный шейдер
          * `builtin`: один из встроенных проходов в движке (draw3D, drawHud, postprocess, ...)
        * `pipeline.get_render_pass_chain()`
          * Returns the table of all current render passes chain of the pipeline in the format:

            ```lua
            {
                {
                    type = "3d"/"postprocess"/"builtin",
                    name = <string>, -- <modname:name> for "3d" and "postprocess"
                    order = <integer>, -- order number of the given pass
                    enable = true/false, -- enable or disable the given pass?
                },
                ...
            }
            ```

        * `pipeline.set_render_pass_chain(table)`
        * `pipeline.enable_render_pass(name, bool)` -- enable or disable the given render pass
        * `pipeline.add_texture_buffer(name, list)`
          * Creates new textures definitions and GPU textures themselves
          * Each texture def looks like:

            ```lua
            {
                type = "2d"/"cubemap"/"array",
                name = <string>, -- texture name
                -- Resolution in pixels, third coord is the texture array layers count if type="array"
                resolution = {x=<number>, y=<number>, [z=<integer>]},
                format = "argb8"/"rgb8"/"d16"/"d32"/"d24s8", -- texture pixel format
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
                },
                msaa = <number>
            }
            ```

        * `pipeline.add_3d_step(<Material table>)`
        * `pipeline.get_posteffects()`
          * Returns the consequent list of all screen effects (bloom, distance-of-field, reflections and etc):

          ```lua
          {
        	  {
        	  	  name = <string>,
        	  	  shader = <file>/<code>, -- fragment
        	  	  uniforms = <list>,
        	  	  texture_outputs = <Texture outputs>,
        	  	  viewport = {x1=..., y1=..., x2=..., y2=...},
        	  	  cliprect = {x1=..., y1=..., x2=..., y2=...},
                  blend = {<Blend table>},
                  line_width = <number>
        	  }
          }
          ```

        * `pipeline.add_posteffect(<Posteffect table>)` -- adds a new post effect
        * `pipeline.override_builtin_posteffect(<Posteffect table>)` -- overrides the builtin effects like bloom, godrays, fsaa and etc

    * Рендеринг примитивов (таблица drawer):
         * `drawer.set_material(<Material table>)`
           * Sets the current material used by the following drawer calls
         * `drawer.set_transform(<Transform matrices>)`
           * Sets the current `world`, `view` and `projection` matrices
             used by the following drawer calls
         * `drawer.draw_list(<Params table>)`
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

         * `drawer.draw_rect(<Params table>)`
           * Params table:

             ```lua
             {
                rect = {x1 = ..., y1 = ..., x2 = ..., y2 = ...},
                color = <ColorSpec or list of ColorSpec>,
             }
             ```

         * `drawer.draw_image(<Params table>)` -- requires the Image API
           * Params table:

             ```lua
             {
                image = <Image object>,
                rect = {x1 = ..., y1 = ..., x2 = ..., y2 = ...},
                color = <ColorSpec or list of ColorSpec>,
                uvs = {x1 = ..., y1 = ..., x2 = ..., y2 = ...}
             }
             ```

         * `drawer.draw_mesh(<Mesh table>)`:
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
        * Будет пропускаться в `drawer.draw_mesh`, в поле `mesh` NodeDef, EntityDef

* Использовать aux1 аттрибут для хранения hardware цвета для блоков colorwallmounted, colorfacedir, color и тд.

* Адаптировать поддержку атласов из old ветки
	* Добавить предзагрузку тайлов, которые будут упакованы в атлас, через atlas.json файл в модах

* Рендерить отдельную копию блока с крэками через режим overlay (#15287)

* Адаптировать hardware скининг из old ветки. Нереализованные изменения:
   1. Lua API для скелета и анимаций сущностей
   2. Другие виды интерполяций кадров (constant, sine, cubic)

* Добавить поддержку цветного воксельного освещения с добавлением u16 param3 (R, G, B по 4 бита)

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
