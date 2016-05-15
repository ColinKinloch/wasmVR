#!/usr/bin/env sh
emmake make clean
emmake make -j 8
mv wasmVR wasmVR.bc
emcc wasmVR.bc --prefix=$(pwd)/system -L./system/lib -lvorbisfile -lvorbis -logg -o wasmVR.js --bind -s DEMANGLE_SUPPORT=1 -O2
python ../emsdk/emscripten/incoming/tools/file_packager.py data.data --preload data --js-output=data.js
