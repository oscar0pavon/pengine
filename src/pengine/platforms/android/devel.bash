if [ ! -d ~/PavonEngine/build ]
then
  echo Creating Engine build directory && mkdir ~/PavonEngine/build && cd ~/PavonEngine/build && cmake .. 
else
  cd ~/PavonEngine/build
fi

make -j8

cd ~/PavonEngine/source/engine/platforms/android/

echo // >> main.c

if [ ! -d ~/PavonEngine/source/engine/platforms/android/build ]
then
  echo Creating Android build directory && mkdir ~/PavonEngine/source/engine/platforms/android/build 
  cd ~/PavonEngine/source/engine/platforms/android/build && cmake ..
else
  cd ~/PavonEngine/source/engine/platforms/android/build
fi

make -j8

echo "Compiling shaders..."
cd ~/PavonEngine/NativeContent/shaders/
./vk_compile.sh

cd ~/PavonEngine/source/engine/platforms/android/

./apk_make.sh

###Installation process
echo "Installing APK..."
sudo pm install ./pavons.apk #with root
#adb install ./pavons.apk #without root on android devices
#fakeroot adb install -r --fastdeploy ./pavons.apk

echo "Starting APK..."
./start.sh && ./debug.sh
#am start com.pavonstudios.app/android.app.NativeActivity

