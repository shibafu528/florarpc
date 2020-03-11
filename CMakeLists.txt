cmake_minimum_required(VERSION 3.15)
project(florarpc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GRPC REQUIRED grpc)
pkg_search_module(GRPCPP REQUIRED grpc++)

add_executable(flora
        main.cpp
        resources.qrc
        ui/MainWindow.ui
        ui/MainWindow.cpp
        ui/MainWindow.h
        entity/Protocol.cpp
        entity/Protocol.h
        ui/ProtocolModel.cpp
        ui/ProtocolModel.h
        util/GrpcUtility.cpp
        util/GrpcUtility.h)

target_link_libraries(flora Qt5::Widgets protobuf::libprotobuf grpc++)
