FloraRPC
====

QtでgRPC Clientを書いてみる試み。Awesome gRPCを見てもネイティブのデスクトップGUIツールキットを使った実装が全然無かったので……。

## Dependencies
- Qt 5.14
- protobuf 3.11
- gRPC 1.27
- KSyntaxHighlighting 5.67

## Build (for Arch Linux)

```sh
$ yay -S cmake qt5-base protobuf grpc syntax-highlighting
$ git submodule update --init
$ cmake .
$ make
```

## Build (for Windows)
### Additional prerequisites
* Perl (ex. http://strawberryperl.com/)
* vcpkg (https://github.com/microsoft/vcpkg)

```bat
REM // install dependencies with vcpkg
%VCPKG_DIR%\vcpkg.exe --triplet x64-windows install protobuf grpc ecm

REM // add Qt5 bin directory to PATH (use to build KF5SyntaxHighlighting)
REM // (QTDIR -> ex. C:\Qt\Qt5.14.1\5.14.1\msvc2017_64)
set PATH=%QTDIR%\bin;%PATH%

REM // change directory to florarpc\build, and make
cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=%QTDIR% -DCMAKE_BUILD_TYPE=Release -A x64 ..
msbuild /p:Configuration=Release ALL_BUILD.vcxproj

REM // copy Qt5 dlls
%QTDIR%\bin\windeployqt -release Release\flora.exe
copy /Y %QTDIR%\bin\Qt5Network.dll Release\

REM // copy Qt5 translations
copy /Y %QTDIR%\translations\qtbase_*.qm Release\translations\

REM // copy KF5SyntaxHighlighting dll
copy /Y bin\Release\KF5SyntaxHighlighting.dll Release\
```

## Build (for macOS)
### Additional prerequisites
* vcpkg (https://github.com/microsoft/vcpkg)

```sh
# install dependencies with vcpkg
${VCPKG_DIR}/vcpkg install protobuf grpc ecm

# change directory to florarpc/build, and make
cmake -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=${QTDIR} ..
make

# copy Qt5 dylibs
${QTDIR}/bin/macdeployqt flora.app -always-overwrite

# copy Qt5 translations
mkdir -p flora.app/Contents/translations
cp -f ${QTDIR}/translations/qtbase_*.qm flora.app/Contents/translations
```
