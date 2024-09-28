s6828 количество контента нужно минимизировать до одной фичи. То исмь графику исключать всю что возможно. врп.

also. сборка на windows оказалась чем-то непостижимым. couch coop тому виной. Деустановить CC возможности нет без заметных финансовложений. мв.

# tynmaze-fps


[forked from tynmaze-fps](https://github.com/tynrare/tynmaze-auto)
[web](https://tynmaze-auto.netlify.app/)

# build

- Raylib has to be compiled with `cmake -DGRAPHICS=GRAPHICS_API_OPENGL_ES2 ..` option to work properly with glsl 100 version
- To build release version run `cmake -DCMAKE_BUILD_TYPE=Release ..`
- By default, debug version 'res' folder used directly. In release version 'build/res' directory used.
