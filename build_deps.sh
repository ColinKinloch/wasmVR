#!/usr/bin/env sh
dir=$(pwd)
deps=$dir/deps
prefix="$dir/system"

git submodule init
git submodule update

cd $deps

cd ogg
./autogen.sh
sed -i "s/-O20/-O2/g" configure
emconfigure ./configure --prefix=$prefix --disable-static
emmake make clean
emmake make
emmake make install
cd $deps


cd vorbis
sed -i 's/$srcdir\/configure/#/' autogen.sh
./autogen.sh
sed -i "s/-O20/-O2/g" configure
sed -i '' 's/$ac_cv_func_oggpack_writealign/yes/' configure
emconfigure ./configure --prefix=$prefix --disable-oggtest --disable-static
#--with-ogg=$dir/libogg --with-ogg-libraries=$dir/libogg/lib
emmake make clean
emmake make
emmake make install
cd $deps

cd glm
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=$prefix ..
emmake make clean
emmake make
emmake make install
cd $deps

cd $dir

cp $deps/json/src/json.hpp $prefix/include/
