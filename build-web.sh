path_raylib=../../raylib
path_emsdk=../../emsdk

source "$path_emsdk/emsdk_env.sh"
rm -rf pub/web
mkdir -p pub/web


emcc src/*.c -g3 \
	-o pub/web/index.html \
	--shell-file shell_minimal.html \
	--preload-file res \
	-Os -Wall $path_raylib/src/libraylib.a -DPLATFORM_WEB \
	-I. -I$path_raylib/src \
	-L. -L$path_raylib/src \
	-Wno-incompatible-function-pointer-types \
	-s USE_GLFW=3 -s ASYNCIFY
