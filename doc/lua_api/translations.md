Translations
============

Texts can be translated client-side with the help of `core.translate` and
translation files.

Translating a string
--------------------

Two functions are provided to translate strings: `core.translate` and
`core.get_translator`.

* `core.get_translator(textdomain)` is a simple wrapper around
  `core.translate` and `core.translate_n`.
  After `local S, PS = core.get_translator(textdomain)`, we have
  `S(str, ...)` equivalent to `core.translate(textdomain, str, ...)`, and
  `PS(str, str_plural, n, ...)` to `core.translate_n(textdomain, str, str_plural, n, ...)`.
  It is intended to be used in the following way, so that it avoids verbose
  repetitions of `core.translate`:

  ```lua
  local S, PS = core.get_translator(textdomain)
  S(str, ...)
  ```

  As an extra commodity, if `textdomain` is nil, it is assumed to be "" instead.

* `core.translate(textdomain, str, ...)` translates the string `str` with
  the given `textdomain` for disambiguation. The textdomain must match the
  textdomain specified in the translation file in order to get the string
  translated. This can be used so that a string is translated differently in
  different contexts.
  It is advised to use the name of the mod as textdomain whenever possible, to
  avoid clashes with other mods.
  This function must be given a number of arguments equal to the number of
  arguments the translated string expects.
  Arguments are literal strings -- they will not be translated.

* `core.translate_n(textdomain, str, str_plural, n, ...)` translates the
  string `str` with the given `textdomain` for disambiguaion. The value of
  `n`, which must be a nonnegative integer, is used to decide whether to use
  the singular or the plural version of the string. Depending on the locale of
  the client, the choice between singular and plural might be more complicated,
  but the choice will be done automatically using the value of `n`.

  You can read https://www.gnu.org/software/gettext/manual/html_node/Plural-forms.html
  for more details on the differences of plurals between languages.

  Also note that plurals are only handled in .po or .mo files, and not in .tr files.

For instance, suppose we want to greet players when they join and provide a
command that shows the amount of time since the player joined. We can do the
following:

```lua
local S, PS = core.get_translator("hello")
core.register_on_joinplayer(function(player)
    local name = player:get_player_name()
    core.chat_send_player(name, S("Hello @1, how are you today?", name))
end)
core.register_chatcommand("playtime", {
    func = function(name)
        local last_login = core.get_auth_handler().get_auth(name).last_login
        local playtime = math.floor((last_login-os.time())/60)
        return true, PS(
            "You have been playing for @1 minute.",
            "You have been playing for @1 minutes.",
            minutes, tostring(minutes))
    end,
})
```

When someone called "CoolGuy" joins the game with an old client or a client
that does not have localization enabled, they will see `Hello CoolGuy, how are
you today?`. If they use the `/playtime` command, they will see `You have been
playing for 1 minute` or (for example) `You have been playing for 4 minutes.`

However, if we have for instance a translation file named `hello.de.po`
containing the following:

```po
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);\n"

msgid "Hello @1, how are you today?"
msgstr "Hallo @1, wie geht es dir heute?"

msgid "You have been playing for @1 minute."
msgid_plural "You have been playing for @1 minutes."
msgstr[0] "Du spielst seit @1 Minute."
msgstr[1] "Du spielst seit @1 Minuten."
```

and CoolGuy has set a German locale, they will see `Hallo CoolGuy, wie geht es
dir heute?` when they join, and the `/playtime` command will show them `Du
spielst seit 1 Minute.` or (for example) `Du spielst seit 4 Minuten.`

Creating and updating translation files
---------------------------------------

As an alternative to writing translation files by hand (as shown in the above
example), it is also possible to generate translation files based on the source
code.

It is recommended to first generate a translation template. The translation
template includes translatable strings that translators can directly work on.
After creating the `locale` directory, a translation template for the above
example using the following command:

```sh
xgettext -L lua -kS -kPS:1,2 -kcore.translate:1c,2 -kcore.translate_n:1c,2,3 \
  -d hello -o locale/hello.pot *.lua
```

The above command can also be used to update the translation template when new
translatable strings are added.

The German translator can then create the translation file with

```sh
msginit -l de -i locale/hello.pot -o locale/hello.de.po
```

and provide the translations by editing `locale/hello.de.po`.

The translation file can be updated using

```sh
msgmerge -U locale/hello.de.po locale/hello.pot
```

Refer to the [Gettext manual](https://www.gnu.org/software/gettext/manual/) for
further information on creating and updating translation files.

Operations on translated strings
--------------------------------

The output of `core.translate` is a string, with escape sequences adding
additional information to that string so that it can be translated on the
different clients. In particular, you can't expect operations like string.length
to work on them like you would expect them to, or string.gsub to work in the
expected manner. However, string concatenation will still work as expected
(note that you should only use this for things like formspecs; do not translate
sentences by breaking them into parts; arguments should be used instead), and
operations such as `core.colorize` which are also concatenation.

Old translation file format
---------------------------

A translation file has the suffix `.[lang].tr`, where `[lang]` is the language
it corresponds to. It must be put into the `locale` subdirectory of the mod.
The file should be a text file, with the following format:

* Lines beginning with `# textdomain:` (the space is significant) can be used
  to specify the text domain of all following translations in the file.
* All other empty lines or lines beginning with `#` are ignored.
* Other lines should be in the format `original=translated`. Both `original`
  and `translated` can contain escape sequences beginning with `@` to insert
  arguments, literal `@`, `=` or newline (See [Escapes] below).
  There must be no extraneous whitespace around the `=` or at the beginning or
  the end of the line.

Using the earlier example of greeting the player, the translation file would be

```
# textdomain: hello
Hello @1, how are you today?=Hallo @1, wie geht es dir heute?
```

For old translation files, consider using the script `mod_translation_updater.py`
in the Luanti [modtools](https://github.com/minetest/modtools) repository to
generate and update translation files automatically from the Lua sources.

Gettext translation file format
-------------------------------

Gettext files can also be used as translations. A translation file has the suffix
`.[lang].po` or `.[lang].mo`, depending on whether it is compiled or not, and must
also be placed in the `locale` subdirectory of the mod. The value of `textdomain`
is `msgctxt` in the gettext files. If `msgctxt` is not provided, the name of the
translation file is used instead.

A typical entry in a `.po` file would look like:

```po
msgctxt "textdomain"
msgid "Hello world!"
msgstr "Bonjour le monde!"
```

Escapes
-------

Strings that need to be translated can contain several escapes, preceded by `@`.

* `@@` acts as a literal `@`.
* `@n`, where `n` is a digit between 1 and 9, is an argument for the translated
  string that will be inlined when translated. Due to how translations are
  implemented, the original translation string **must** have its arguments in
  increasing order, without gaps or repetitions, starting from 1.
* `@=` acts as a literal `=`. It is not required in strings given to
  `core.translate`, but is in translation files to avoid being confused
  with the `=` separating the original from the translation.
* `@\n` (where the `\n` is a literal newline) acts as a literal newline.
  As with `@=`, this escape is not required in strings given to
  `core.translate`, but is in translation files.
* `@n` acts as a literal newline as well.

Server side translations
------------------------

On some specific cases, server translation could be useful. For example, filter
a list on labels and send results to client. A method is supplied to achieve
that:

`core.get_translated_string(lang_code, string)`: resolves translations in
the given string just like the client would, using the translation files for
`lang_code`. For this to have any effect, the string needs to contain translation
markup, e.g. `core.get_translated_string("fr", S("Hello"))`.

The `lang_code` to use for a given player can be retrieved from
the table returned by `core.get_player_information(name)`.

IMPORTANT: This functionality should only be used for sorting, filtering or similar purposes.
You do not need to use this to get translated strings to show up on the client.

Translating content meta
------------------------

You can translate content meta, such as `title` and `description`, by placing
translations in a `locale/DOMAIN.LANG.tr` file. The textdomain defaults to the
content name, but can be customised using `textdomain` in the content's .conf.

### Mods and Texture Packs

Say you have a mod called `mymod` with a short description in mod.conf:

```
description = This is the short description
```

Luanti will look for translations in the `mymod` textdomain as there's no
textdomain specified in mod.conf. For example, `mymod/locale/mymod.fr.tr`:

```
# textdomain:mymod
This is the short description=Voici la description succincte
```

### Games and Modpacks

For games and modpacks, Luanti will look for the textdomain in all mods.

Say you have a game called `mygame` with the following game.conf:

```
description = This is the game's short description
textdomain = mygame
```

Luanti will then look for the textdomain `mygame` in all mods, for example,
`mygame/mods/anymod/locale/mygame.fr.tr`. Note that it is still recommended that your
textdomain match the mod name, but this isn't required.
