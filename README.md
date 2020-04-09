FloraRPC
====

QtでgRPC Clientを書いてみる試み。Awesome gRPCを見てもネイティブのデスクトップGUIツールキットを使った実装が全然無かったので……。

## Dependencies
- Qt 5.14
- protobuf 3.11
- gRPC 1.27
- KSyntaxHighlighting 5.66

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

```bat
git submodule update --init

REM // install dependencies with vcpkg
nmake install_deps

REM // build
nmake dev
```

## Build (for macOS)

```sh
git submodule update --init

# install dependencies with vcpkg
make install_deps

# build
make dev
```
