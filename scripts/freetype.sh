if [ ! -d ./freetype ]
then
git clone https://github.com/freetype/freetype
fi
cd freetype
./autogen.sh
./configure --with-zlib=no --with-bzip2=no --with-brotli=no --with-png=no CFLAGS="-fPIC"
make -j8
cp ./objs/.libs/libfreetype.a ~/PavonEngine/lib

