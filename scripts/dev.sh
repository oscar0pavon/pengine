rm ~/PavonEngine/bin/peditorexec

if [ ! -d ~/PavonEngine/build ]
then
  echo Creating Engine build directory && mkdir ~/PavonEngine/build && cd ~/PavonEngine/build && cmake .. 
else
  cd ~/PavonEngine/build
fi

make -j32


echo "Compiling shaders..."
cd ~/PavonEngine/NativeContent/shaders/
./vk_compile.sh

cd ~/PavonEngine
./configure_content.sh

./bin/peditorexec
