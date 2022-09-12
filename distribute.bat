@call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
@echo off

set version=2.2.5

call msbuild -m:5 -nologo -p:Configuration="Release" -p:Platform="x86" -t:Clean;Rebuild
call msbuild -m:5 -nologo -p:Configuration="Debug" -p:Platform="x86" -t:Clean;Rebuild
call msbuild -m:5 -nologo -p:Configuration="Release" -p:Platform="x64" -t:Clean;Rebuild
call msbuild -m:5 -nologo -p:Configuration="Debug" -p:Platform="x64" -t:Clean;Rebuild

mkdir distribute\nes_ntsc-"%version%"-x86\Release
copy bin\x86\Release distribute\nes_ntsc-"%version%"-x86\Release
copy blargg_ntsc.txt distribute\nes_ntsc-"%version%"-x86\Release
copy changes.txt distribute\nes_ntsc-"%version%"-x86\Release
copy license.txt distribute\nes_ntsc-"%version%"-x86\Release
copy readme.txt distribute\nes_ntsc-"%version%"-x86\Release
copy test.bmp distribute\nes_ntsc-"%version%"-x86\Release
mkdir distribute\nes_ntsc-"%version%"-x86\Debug
copy bin\x86\Debug distribute\nes_ntsc-"%version%"-x86\Debug
copy blargg_ntsc.txt distribute\nes_ntsc-"%version%"-x86\Debug
copy changes.txt distribute\nes_ntsc-"%version%"-x86\Debug
copy license.txt distribute\nes_ntsc-"%version%"-x86\Debug
copy readme.txt distribute\nes_ntsc-"%version%"-x86\Debug
copy test.bmp distribute\nes_ntsc-"%version%"-x86\Debug

mkdir distribute\nes_ntsc-"%version%"-x64\Release
copy bin\x64\Release distribute\nes_ntsc-"%version%"-x64\Release
copy blargg_ntsc.txt distribute\nes_ntsc-"%version%"-x64\Release
copy changes.txt distribute\nes_ntsc-"%version%"-x64\Release
copy license.txt distribute\nes_ntsc-"%version%"-x64\Release
copy readme.txt distribute\nes_ntsc-"%version%"-x64\Release
copy test.bmp distribute\nes_ntsc-"%version%"-x64\Release
mkdir distribute\nes_ntsc-"%version%"-x64\Debug
copy bin\x64\Debug distribute\nes_ntsc-"%version%"-x64\Debug
copy blargg_ntsc.txt distribute\nes_ntsc-"%version%"-x64\Debug
copy changes.txt distribute\nes_ntsc-"%version%"-x64\Debug
copy license.txt distribute\nes_ntsc-"%version%"-x64\Debug
copy readme.txt distribute\nes_ntsc-"%version%"-x64\Debug
copy test.bmp distribute\nes_ntsc-"%version%"-x64\Debug

cd distribute
cd nes_ntsc-"%version%"-x86
call 7z a -t7z -mx=9 -mmt=3 -m0=LZMA2:d=26:fb=128 -ms=on ..\nes_ntsc-"%version%"-x86.7z
cd ..
cd nes_ntsc-"%version%"-x64
call 7z a -t7z -mx=9 -mmt=3 -m0=LZMA2:d=26:fb=128 -ms=on ..\nes_ntsc-"%version%"-x64.7z 
cd ..
cd ..
@pause