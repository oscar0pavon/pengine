if [ ! -r /sdcard/Download/PavonEngine ]
then
  mkdir /sdcard/Download/PavonEngine
  mkdir /sdcard/Download/PavonEngine/shaders
  cp ~/PavonEngine/NativeContent/shaders/* /sdcard/Download/PavonEngine/shaders
else
  cp ~/PavonEngine/NativeContent/shaders/* /sdcard/Download/PavonEngine/shaders
fi

#chess content
if [ ! -r /sdcard/Download/chess ]
then
  mkdir /sdcard/Download/chess
  cp ~/PavonEngine/demos/chess/*.glb /sdcard/Download/chess
else
  cp ~/PavonEngine/demos/chess/*.glb /sdcard/Download/chess
fi

if [ ! -r /sdcard/Download/NativeContent ]
then
cp -r ~/PavonEngine/NativeContent /sdcard/Download
else
cp -r ~/PavonEngine/NativeContent /sdcard/Download
fi

echo "Contents copy to /sdcard/Download"


