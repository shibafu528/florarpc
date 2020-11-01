cmake_minimum_required(VERSION 3.12)
project(florarpc VERSION 0.1.8)

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0601)
  # https://stackoverflow.com/a/47704050
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

find_package(Qt5 COMPONENTS Widgets Qml REQUIRED)

option(FLORA_FORCE_BUILD_KF5LIB "システムのKF5ライブラリを使用せず、常にビルドする")
if(NOT FLORA_FORCE_BUILD_KF5LIB)
    find_package(KF5SyntaxHighlighting)
endif()
if(FLORA_FORCE_BUILD_KF5LIB OR (NOT KF5SyntaxHighlighting_FOUND))
    set(BUILD_HTML_DOCS OFF CACHE INTERNAL _)
    set(BUILD_MAN_DOCS OFF CACHE INTERNAL _)
    set(BUILD_QTHELP_DOCS OFF CACHE INTERNAL _)
    set(BUILD_TESTING OFF CACHE INTERNAL _)
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
    set(LIB_gRPC gRPC::grpc gRPC::grpc++)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GRPC REQUIRED grpc)
    pkg_search_module(GRPCPP REQUIRED grpc++)
    set(LIB_gRPC grpc grpc++)
endif()

set(FLORA_VERSION_MAJOR ${florarpc_VERSION_MAJOR})
set(FLORA_VERSION_MINOR ${florarpc_VERSION_MINOR})
set(FLORA_VERSION_PATCH ${florarpc_VERSION_PATCH})
if("${florarpc_VERSION_TWEAK}" STREQUAL "")
    set(FLORA_VERSION_TWEAK 0)
else()
    set(FLORA_VERSION_TWEAK ${florarpc_VERSION_TWEAK})
endif()
configure_file(${PROJECT_SOURCE_DIR}/flora_constants.h.in ${PROJECT_BINARY_DIR}/flora_generated/flora_constants.h)

set(GOOGLEAPIS_PROTOBUF_INCLUDE "${PROJECT_SOURCE_DIR}/vendor/googleapis")
set(FLORA_PROTOBUF_INCLUDE "${PROJECT_SOURCE_DIR}/proto")
set(FLORA_PROTOBUF_GENERATED "${PROJECT_BINARY_DIR}/proto_generated")
set(FLORA_PROTOBUF_PROTOS
        ${FLORA_PROTOBUF_INCLUDE}/florarpc/descriptor_exports.proto
        ${FLORA_PROTOBUF_INCLUDE}/florarpc/version.proto
        ${FLORA_PROTOBUF_INCLUDE}/florarpc/preferences.proto
        ${FLORA_PROTOBUF_INCLUDE}/florarpc/workspace.proto
        ${GOOGLEAPIS_PROTOBUF_INCLUDE}/google/rpc/code.proto
        ${GOOGLEAPIS_PROTOBUF_INCLUDE}/google/rpc/error_details.proto
        ${GOOGLEAPIS_PROTOBUF_INCLUDE}/google/rpc/status.proto)
set(FLORA_PROTOBUF_SOURCES
        ${FLORA_PROTOBUF_GENERATED}/florarpc/descriptor_exports.pb.cc
        ${FLORA_PROTOBUF_GENERATED}/florarpc/version.pb.cc
        ${FLORA_PROTOBUF_GENERATED}/florarpc/preferences.pb.cc
        ${FLORA_PROTOBUF_GENERATED}/florarpc/workspace.pb.cc
        ${FLORA_PROTOBUF_GENERATED}/google/rpc/code.pb.cc
        ${FLORA_PROTOBUF_GENERATED}/google/rpc/error_details.pb.cc
        ${FLORA_PROTOBUF_GENERATED}/google/rpc/status.pb.cc)
set(FLORA_PROTOBUF_HEADERS
        ${FLORA_PROTOBUF_GENERATED}/florarpc/descriptor_exports.pb.h
        ${FLORA_PROTOBUF_GENERATED}/florarpc/version.pb.h
        ${FLORA_PROTOBUF_GENERATED}/florarpc/preferences.pb.h
        ${FLORA_PROTOBUF_GENERATED}/florarpc/workspace.pb.h
        ${FLORA_PROTOBUF_GENERATED}/google/rpc/code.pb.h
        ${FLORA_PROTOBUF_GENERATED}/google/rpc/error_details.pb.h
        ${FLORA_PROTOBUF_GENERATED}/google/rpc/status.pb.h)
file(MAKE_DIRECTORY "${FLORA_PROTOBUF_GENERATED}")
add_custom_command(
        OUTPUT
        ${FLORA_PROTOBUF_SOURCES}
        ${FLORA_PROTOBUF_HEADERS}
        COMMAND
        ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS
        --cpp_out "${FLORA_PROTOBUF_GENERATED}"
        -I "${FLORA_PROTOBUF_INCLUDE}"
        -I "${GOOGLEAPIS_PROTOBUF_INCLUDE}"
        ${FLORA_PROTOBUF_PROTOS}
        MAIN_DEPENDENCY
        ${FLORA_PROTOBUF_PROTOS})

if(WIN32)
    set(FLORA_PLATFORM_SOURCES
            resources.rc
            platform/RootCertificates.h
            platform/windows/RootCertificates.cpp)
endif()

if(APPLE)
    set(FLORA_PLATFORM_SOURCES
            resources/appicon/FloraRPC.icns
            platform/RootCertificates.h
            platform/mac/NSWindow.h
            platform/mac/NSWindow.mm
            platform/mac/RootCertificates.mm)
endif()

add_executable(flora
        WIN32
        MACOSX_BUNDLE
        main.cpp
        flora_constants.h
        resources.qrc
        ui/AboutDialog.ui
        ui/AboutDialog.cpp
        ui/AboutDialog.h
        ui/CertsEditControl.ui
        ui/CertsEditControl.cpp
        ui/CertsEditControl.h
        ui/CertsEditDialog.ui
        ui/CertsEditDialog.cpp
        ui/CertsEditDialog.h
        ui/MainWindow.ui
        ui/MainWindow.cpp
        ui/MainWindow.h
        ui/MetadataEdit.ui
        ui/MetadataEdit.cpp
        ui/MetadataEdit.h
        ui/ImportsManageDialog.ui
        ui/ImportsManageDialog.cpp
        ui/ImportsManageDialog.h
        ui/Editor.ui
        ui/Editor.cpp
        ui/Editor.h
        ui/MultiPageJsonView.ui
        ui/MultiPageJsonView.cpp
        ui/MultiPageJsonView.h
        ui/ServerEditDialog.ui
        ui/ServerEditDialog.cpp
        ui/ServerEditDialog.h
        ui/ServersManageDialog.ui
        ui/ServersManageDialog.cpp
        ui/ServersManageDialog.h
        entity/Certificate.cpp
        entity/Certificate.h
        entity/Protocol.cpp
        entity/Protocol.h
        entity/Preferences.cpp
        entity/Preferences.h
        entity/Metadata.cpp
        entity/Metadata.h
        entity/Method.cpp
        entity/Method.h
        entity/Session.cpp
        entity/Session.h
        entity/Server.cpp
        entity/Server.h
        ui/event/WorkspaceModifiedEvent.cpp
        ui/event/WorkspaceModifiedEvent.h
        ui/task/ImportProtosTask.cpp
        ui/task/ImportProtosTask.h
        ui/ProtocolTreeModel.cpp
        ui/ProtocolTreeModel.h
        util/DescriptorPoolProxy.cpp
        util/DescriptorPoolProxy.h
        util/GrpcUtility.cpp
        util/GrpcUtility.h
        util/ProtobufIterator.h
        util/ProtobufJsonPrinter.cpp
        util/ProtobufJsonPrinter.h
        util/SyntaxHighlighter.cpp
        util/SyntaxHighlighter.h
        ${FLORA_PROTOBUF_SOURCES}
        ${FLORA_PROTOBUF_HEADERS}
        ${FLORA_PLATFORM_SOURCES})

target_link_libraries(flora
        Qt5::Widgets
        Qt5::Qml
        KF5::SyntaxHighlighting
        protobuf::libprotobuf
        ${LIB_gRPC})

if(WIN32)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(flora OpenSSL::Crypto crypt32)
endif()

target_include_directories(flora
        PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/vendor/minijson_writer
        ${PROJECT_BINARY_DIR}/flora_generated
        ${FLORA_PROTOBUF_GENERATED})

if(APPLE)
    find_library(APPKIT_LIBRARY AppKit)
    find_library(SECURITY_LIBRARY Security)
    target_link_libraries(flora ${APPKIT_LIBRARY} ${SECURITY_LIBRARY})
    set_target_properties(flora PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/AppleInfo.plist)
    set_source_files_properties(resources/appicon/FloraRPC.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_property(TARGET flora APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()
