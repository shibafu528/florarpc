# Makefile for Microsoft nmake

!IFNDEF Qt5_DIR
Qt5_DIR    = C:\Qt\Qt5.14.1\5.14.1\msvc2017_64
!ENDIF
!IFNDEF VCPKG_ROOT
VCPKG_ROOT = vendor\vcpkg
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

release: build-release
	pushd build-release && \
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)\scripts\buildsystems\vcpkg.cmake \
		-DCMAKE_PREFIX_PATH=$(Qt5_DIR) -A x64 .. && \
	cmake --build . --config Release && \
	$(Qt5_DIR)\bin\windeployqt -release Release\flora.exe && \
	copy /Y $(Qt5_DIR)\bin\Qt5Network.dll Release && \
	copy /Y $(Qt5_DIR)\translations\qtbase_*.qm Release\translations && \
	copy /Y bin\Release\KF5SyntaxHighlighting.dll Release && \
	popd

install_deps: $(VCPKG_ROOT)\vcpkg.exe
	$(VCPKG_ROOT)\vcpkg install --triplet x64-windows @vcpkg_packages.txt

$(VCPKG_ROOT)\vcpkg.exe:
	call $(VCPKG_ROOT)\bootstrap-vcpkg.bat

build:
	mkdir build

build-release:
	mkdir build-release

clean:
	rd /S /Q build build-release 2> nul || type nul > nul

.PHONY: dev clean install_deps
