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

 Адаптировать SSCSM выполнение от luk3yx: https://gitlab.com/luk3yx/minetest-sscsm

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
                on_render = function() { ... }
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
            uniforms = {
                {name = "cameraPos", type = "vector3d", value = vector.new()},
                {name = "animationTimer", type = "float", value = curTime},
                ...
            }
         }
         ```

      * Коллбэк таблицы (`on_render = function(self)`), позволяющий менять ее параметры в каждом шаге рендера
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

- Добавить в методах ObjectRef возможность изменения позиции/скорости/поворота/масштаба с интерполяциями.

- Рендерить отдельную копию блока с крэками через режим overlay (#15287).

- Адаптировать hardware скининг из old ветки. Нереализованные изменения:
   1. Lua API для скелета и анимаций сущностей
   2. Другие виды интерполяций кадров (constant, sine, cubic)

- Добавить поддержку цветного воксельного освещения с добавлением u16 param3 (R, G, B по 4 бита)

- Новая процедурная генерация текстур (замена текстурных модификаторов). Поддержка различных режимов смешивания (Normal, Alpha, Add, Multiply, Overlay, Screen и тд).

- Добавить API матриц и кватернионов в builtin (#8515)

- Мержить поддержку атласа (#15061) с исправлением оставшихся проблем. Заменить алгоритм упавковки на rectpack2D (https://github.com/TeamHypersomnia/rectpack2D).

- Мержить поддержку эмбиента (#14343).

- Использовать дополнительный аттрибут для хранения hardware цвета для блоков colorwallmounted, colorfacedir, color и тд.

- Поддержка отображения HUD мешей и их кастомизация.

- Dual wielding (#14427).

- API рендера 3D примитивов и простых форм (куб, сфера, цилиндр, параллелепипед и тд). Замена частного случая (3d line rendering #13020).

- Более расширенный API контроля над камерой игрока и возможность создавать вторичные камеры (Camera API #14325).

- Мержить Formspec/HUD Replacement (#14263).

- Различные баги:
   1. visual_size прикрепляемой сущности умножается на visual_size родителя.
   2. Туловище (base) прикрепленного игрока при установке смещения через player:set_attach остается на месте, а смещается только голова с камерой (#10101).
   3. Unload сущности игнорирует аттачменты (#14583).
   4. Тени от аттачментов не движутся с их родительскими сущностями (#12630).
   5. Игрок движется по инерции при зажатых клавишах перемещения если был вызван player:set_physics_override({speed=0}) (#10037).
   6. Рэйкасты работают неправильно на прикрепленных сущностях (#10304).
   7. Не работает смешивание анимаций (переход от одной анимаций к другой) (#14817).
   8. Сущности позади или внутри блоков могут быть указаны игроком (#15599).
   9. После unload сущностей с прикрепленными looped звуками звуки начинают играть везде.
   10. Звук не останавливается после уничтожения сущности к которой он был прикреплен.
