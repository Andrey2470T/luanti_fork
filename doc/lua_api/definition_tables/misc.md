Bit Library
-----------

Functions: bit.tobit, bit.tohex, bit.bnot, bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift, bit.arshift, bit.rol, bit.ror, bit.bswap

See http://bitop.luajit.org/ for advanced information.

Tracy Profiler
--------------

Luanti can be built with support for the Tracy profiler, which can also be
useful for profiling mods and is exposed to Lua as the global `tracy`.

See doc/developing/misc.md for details.

Note: This is a development feature and not covered by compatibility promises.

Error Handling
--------------

When an error occurs that is not caught, Luanti calls the function
`core.error_handler` with the error object as its first argument. The second
argument is the stack level where the error occurred. The return value is the
error string that should be shown. By default this is a backtrace from
`debug.traceback`. If the error object is not a string, it is first converted
with `tostring` before being displayed. This means that you can use tables as
error objects so long as you give them `__tostring` metamethods.

You can override `core.error_handler`. You should call the previous handler
with the correct stack level in your implementation.
