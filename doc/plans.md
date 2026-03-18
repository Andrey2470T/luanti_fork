`Планы в IrrlichtRedo`
----------------------

- Рефакторить Video классы (добавить на замену новые классы из old ветки: DrawContext, ITexture, Texture2D, TextureCubeMap, VAO...)
- Перенести /src/GUI в luanti_fork/src/gui, объединить классы-дупликаты
- Удалить /src/IO, использовать cpp filesystem
- Заменить меш загрузчики на Assimp
- Удалить абстрактные классы из /include, объединив их с классами реализаций
- Распределить /include файлы по папкам

`Планы в Luanti Fork`
---------------------

- Адаптировать SSCSM выполнение от luk3yx: https://gitlab.com/luk3yx/minetest-sscsm

- Добавить графический CSM API. Будет включать:
   - Поддержка материалов через таблицы.
      - Возможно задавать базовые параметры (глубина, смешивание, трафарет, шейдеры, настройки текстур тайлов и тд) и PBR (roughness, metallic, specular, AO, emission и тд):
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
                    color = ColorSpec
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
                    mode = "keep", "zero", "replace"...
                },
                cull = {
                    enable = true/false,
                    mode = "back", "front", "front_and_back"
                },
                viewport = {x1= 0, y1= 0, x2 = 1, y2 = 1},
                line_width = 2.0,
                pbr = {
                    color = ColorSpec,
                    metallic = number/mapname,
                    roughness = number/mapname,
                    ambient_occlusion = number/mapname,
                    normal = mapname,
                    emission = number/mapname,
                    transmission = number,
                    ior = NUMBER
                }
            }
            ...
         }
         ```
      - Таблица `materials` будет доступна в NodeDef, ItemDef, EntityDef, HudDef
      - Подтаблица shaders, задающая вершинный и фрагментый шейдеры:
         ```lua
         shaders = {
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
      - Коллбэк таблицы (`on_render = function(self)`), позволяющий менять ее параметры в каждом шаге рендера
   - Поддержка кастомных шейдеров (через materials и postprocess)
   - Поддержка постэффектов.

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
