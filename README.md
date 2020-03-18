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
