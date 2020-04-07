# Makefile for macOS

.PHONY: dev clean install_deps

UNAME := $(shell uname -s)

ifeq ($(UNAME),Darwin)
Qt5_DIR    := ~/Qt5.14.1/5.14.1/clang_64
VCPKG_ROOT := ~/vcpkg

dev: build
	pushd build && \
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake \
		-DCMAKE_PREFIX_PATH=$(Qt5_DIR) \
		-DCMAKE_BUILD_TYPE=Debug .. && \
	cmake --build . && \
	$(Qt5_DIR)/bin/macdeployqt flora.app -always-overwrite -verbose=2 && \
	mkdir -pv flora.app/Contents/translations && \
	cp -fv $(Qt5_DIR)/translations/qtbase_*.qm flora.app/Contents/translations && \
	popd

install_deps:
	$(VCPKG_ROOT)/vcpkg install @vcpkg_packages.txt
endif

build:
	mkdir build

clean:
	rm -rf build
