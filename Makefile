# Makefile for Microsoft nmake

!IFNDEF Qt5_DIR
Qt5_DIR    = C:\Qt\Qt5.14.1\5.14.1\msvc2017_64
!ENDIF
!IFNDEF VCPKG_ROOT
VCPKG_ROOT = C:\local\vcpkg
!ENDIF

PATH       = $(Qt5_DIR)\bin;$(PATH)

dev: build
	pushd build && \
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)\scripts\buildsystems\vcpkg.cmake \
		-DCMAKE_PREFIX_PATH=$(Qt5_DIR) -A x64 .. && \
	cmake --build . --config Debug && \
	$(Qt5_DIR)\bin\windeployqt -debug Debug\flora.exe && \
	copy /Y $(Qt5_DIR)\bin\Qt5Networkd.dll Debug && \
	copy /Y $(Qt5_DIR)\translations\qtbase_*.qm Debug\translations && \
	copy /Y bin\Debug\KF5SyntaxHighlighting.dll Debug && \
	popd

install_deps:
	$(VCPKG_ROOT)\vcpkg install @vcpkg_packages.txt

build:
	mkdir build

clean:
	rd /S /Q build

.PHONY: dev clean install_deps
