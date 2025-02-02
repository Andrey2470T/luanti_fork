Async environment
=================

The engine allows you to submit jobs to be ran in an isolated environment
concurrently with normal server operation.
A job consists of a function to be ran in the async environment, any amount of
arguments (will be serialized) and a callback that will be called with the return
value of the job function once it is finished.

The async environment does *not* have access to the map, entities, players or any
globals defined in the 'usual' environment. Consequently, functions like
`core.get_node()` or `core.get_player_by_name()` simply do not exist in it.

Arguments and return values passed through this can contain certain userdata
objects that will be seamlessly copied (not shared) to the async environment.
This allows you easy interoperability for delegating work to jobs.

* `core.handle_async(func, callback, ...)`:
    * Queue the function `func` to be ran in an async environment.
      Note that there are multiple persistent workers and any of them may
      end up running a given job. The engine will scale the amount of
      worker threads automatically.
    * When `func` returns the callback is called (in the normal environment)
      with all of the return values as arguments.
    * Optional: Variable number of arguments that are passed to `func`
* `core.register_async_dofile(path)`:
    * Register a path to a Lua file to be imported when an async environment
      is initialized. You can use this to preload code which you can then call
      later using `core.handle_async()`.


### List of APIs available in an async environment

Classes:

* `AreaStore`
* `ItemStack`
* `PerlinNoise`
* `PerlinNoiseMap`
* `PseudoRandom`
* `PcgRandom`
* `SecureRandom`
* `VoxelArea`
* `VoxelManip`
    * only if transferred into environment; can't read/write to map
* `Settings`

Class instances that can be transferred between environments:

* `ItemStack`
* `PerlinNoise`
* `PerlinNoiseMap`
* `VoxelManip`

Functions:

* Standalone helpers such as logging, filesystem, encoding,
  hashing or compression APIs
* `core.register_portable_metatable`
* IPC

Variables:

* `core.settings`
* `core.registered_items`, `registered_nodes`, `registered_tools`,
  `registered_craftitems` and `registered_aliases`
    * with all functions and userdata values replaced by `true`, calling any
      callbacks here is obviously not possible
