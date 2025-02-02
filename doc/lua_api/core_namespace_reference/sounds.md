Sounds
======

* `core.sound_play(spec, parameters, [ephemeral])`: returns a handle
    * `spec` is a `SimpleSoundSpec`
    * `parameters` is a sound parameter table
    * `ephemeral` is a boolean (default: false)
      Ephemeral sounds will not return a handle and can't be stopped or faded.
      It is recommend to use this for short sounds that happen in response to
      player actions (e.g. door closing).
* `core.sound_stop(handle)`
    * `handle` is a handle returned by `core.sound_play`
* `core.sound_fade(handle, step, gain)`
    * `handle` is a handle returned by `core.sound_play`
    * `step` determines how fast a sound will fade.
      The gain will change by this much per second,
      until it reaches the target gain.
      Note: Older versions used a signed step. This is deprecated, but old
      code will still work. (the client uses abs(step) to correct it)
    * `gain` the target gain for the fade.
      Fading to zero will delete the sound.
