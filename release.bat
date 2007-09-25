@echo off
pushd Release
copy ..\README readme.txt
copy ..\COPYING license.txt
copy ..\NEWS changelog.txt
zip -D ..\flvmeta-%1%-win32-bin.zip flvmeta.exe flvdump.exe readme.txt license.txt changelog.txt
popd