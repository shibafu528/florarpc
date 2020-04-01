cmake_minimum_required(VERSION 3.15)
project(florarpc)

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

option(FLORA_FORCE_BUILD_KF5LIB "システムのKF5ライブラリを使用せず、常にビルドする")
if(NOT FLORA_FORCE_BUILD_KF5LIB)
    find_package(KF5SyntaxHighlighting)
endif()
if(FLORA_FORCE_BUILD_KF5LIB OR (NOT KF5SyntaxHighlighting_FOUND))
    set(BUILD_HTML_DOCS OFF)
    set(BUILD_MAN_DOCS OFF)
    set(BUILD_QTHELP_DOCS OFF)
    set(BUILD_TESTING OFF)
    add_subdirectory(vendor/kf5-syntax-highlighting)
    add_library(KF5::SyntaxHighlighting ALIAS KF5SyntaxHighlighting)

    # Copy headers
    file(GLOB KF5SyntaxHighlighting_headers vendor/kf5-syntax-highlighting/src/lib/*.h)
    file(COPY ${KF5SyntaxHighlighting_headers} DESTINATION "${CMAKE_BINARY_DIR}/includes/KSyntaxHighlighting")
    include_directories("${CMAKE_BINARY_DIR}/includes")
endif()

find_package(Protobuf REQUIRED)
find_package(gRPC QUIET)
if(gRPC_FOUND)
    set(LIB_gRPC gRPC::grpc++)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GRPC REQUIRED grpc)
    pkg_search_module(GRPCPP REQUIRED grpc++)
    set(LIB_gRPC grpc++)
endif()

add_executable(flora
        MACOSX_BUNDLE
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
        entity/Session.cpp
        entity/Session.h
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
        ${LIB_gRPC})

target_include_directories(flora PRIVATE ${PROJECT_SOURCE_DIR}/vendor/minijson_writer)

if(APPLE)
    set_target_properties(flora PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/AppleInfo.plist)
endif()
