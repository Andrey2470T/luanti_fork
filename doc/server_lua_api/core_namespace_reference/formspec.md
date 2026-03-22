Formspec
========

* `core.show_formspec(playername, formname, formspec)`
    * `playername`: name of player to show formspec
    * `formname`: name passed to `on_player_receive_fields` callbacks.
      It should follow the `"modname:<whatever>"` naming convention.
    * `formname` must not be empty, unless you want to reshow
      the inventory formspec without updating it for future opens.
    * `formspec`: formspec to display
* `core.close_formspec(playername, formname)`
    * `playername`: name of player to close formspec
    * `formname`: has to exactly match the one given in `show_formspec`, or the
      formspec will not close.
    * calling `show_formspec(playername, formname, "")` is equal to this
      expression.
    * to close a formspec regardless of the formname, call
      `core.close_formspec(playername, "")`.
      **USE THIS ONLY WHEN ABSOLUTELY NECESSARY!**
* `core.formspec_escape(string)`: returns a string
    * escapes the characters "[", "]", "\", "," and ";", which cannot be used
      in formspecs.
* `core.hypertext_escape(string)`: returns a string
    * escapes the characters "\", "<", and ">" to show text in a hypertext element.
    * not safe for use with tag attributes.
    * this function does not do formspec escaping, you will likely need to do
      `core.formspec_escape(core.hypertext_escape(string))` if the hypertext is
      not already being formspec escaped.
* `core.explode_table_event(string)`: returns a table
    * returns e.g. `{type="CHG", row=1, column=2}`
    * `type` is one of:
        * `"INV"`: no row selected
        * `"CHG"`: selected
        * `"DCL"`: double-click
* `core.explode_textlist_event(string)`: returns a table
    * returns e.g. `{type="CHG", index=1}`
    * `type` is one of:
        * `"INV"`: no row selected
        * `"CHG"`: selected
        * `"DCL"`: double-click
* `core.explode_scrollbar_event(string)`: returns a table
    * returns e.g. `{type="CHG", value=500}`
    * `type` is one of:
        * `"INV"`: something failed
        * `"CHG"`: has been changed
        * `"VAL"`: not changed
* `core.show_death_screen(player, reason)`
    * Called when the death screen should be shown.
    * `player` is an ObjectRef, `reason` is a PlayerHPChangeReason table or nil.
    * By default, this shows a simple formspec with the option to respawn.
      Respawning is done via `ObjectRef:respawn`.
    * You can override this to show a custom death screen.
    * For general death handling, use `core.register_on_dieplayer` instead.
