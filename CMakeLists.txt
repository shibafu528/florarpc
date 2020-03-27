cmake_minimum_required(VERSION 3.12)
project(florarpc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0601)
  # https://stackoverflow.com/a/47704050
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(KF5SyntaxHighlighting REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC)
if(gRPC_FOUND)
  set(gRPC_LINK_NAME gRPC::grpc++)
else()
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GRPC REQUIRED grpc)
  pkg_search_module(GRPCPP REQUIRED grpc++)
  set(gRPC_LINK_NAME grpc++)
endif()

add_executable(flora
        main.cpp
        resources.qrc
        ui/MainWindow.ui
        ui/MainWindow.cpp
        ui/MainWindow.h
        ui/ImportsManageDialog.ui
        ui/ImportsManageDialog.cpp
        ui/ImportsManageDialog.h
        ui/Editor.ui
        ui/Editor.cpp
        ui/Editor.h
        entity/Protocol.cpp
        entity/Protocol.h
        entity/Method.cpp
        entity/Method.h
        ui/ProtocolTreeModel.cpp
        ui/ProtocolTreeModel.h
        util/GrpcUtility.cpp
        util/GrpcUtility.h
        util/ProtobufIterator.h
        util/ProtobufJsonPrinter.cpp
        util/ProtobufJsonPrinter.h)

target_link_libraries(flora
        Qt5::Widgets
        KF5::SyntaxHighlighting
        protobuf::libprotobuf
        ${gRPC_LINK_NAME})

target_include_directories(flora PRIVATE ${PROJECT_SOURCE_DIR}/vendor/minijson_writer)
