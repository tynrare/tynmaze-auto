# tynmaze-fps


[forked from tynmaze](https://github.com/tynrare/tynmaze)
[web](https://tynmaze-fps.netlify.app/)

# build

- Raylib has to be compiled with `cmake -DGRAPHICS=GRAPHICS_API_OPENGL_ES2 ..` option to work properly with glsl 100 version
- To build release version run `cmake -DCMAKE_BUILD_TYPE=Release ..`
- By default, debug version 'res' folder used directly. In release version 'build/res' directory used.
