# Meteobench top-level Makefile
#
#   make          - regular (dynamic, Debug) dev build in ./build using system Qt
#   make dist     - static release executables for every platform in ./build/dist:
#                     meteobench_linux
#                     meteobench_windows.exe
#                     meteobench_macos_intel
#                     meteobench_macos_apple_silicon
#
# `make dist` bootstraps everything it needs into $(CACHE) (no root required):
#   * qtbase $(QT_VER) source, built statically once per target
#   * llvm-mingw toolchain for the Windows cross build
#   * a macOS SDK for the macOS cross builds (system clang + ld64.lld)
#   * an Ubuntu 22.04 rootfs for the Linux build, entered rootless via user
#     namespaces (scripts/jammy-run). Building against glibc 2.35 with bundled
#     image/text libs and static libstdc++ makes meteobench_linux run on any
#     distro from ~2022 on (Ubuntu 22.04+, Debian 12+, Fedora 36+, ...).
# The first run builds qtbase four times and takes a while; later runs only
# rebuild meteobench. Cross builds use the system Qt ($(HOST_QT)) as host tools,
# so its version must match QT_VER.
#
# Targets run one at a time (.NOTPARALLEL): each qtbase build already uses
# $(JOBS) cores, and four at once will exhaust RAM.
#
# Note: the cache path must not contain spaces (make limitation).

QT_VER        := 6.12.0
QT_MINOR      := $(word 1,$(subst ., ,$(QT_VER))).$(word 2,$(subst ., ,$(QT_VER)))
LLVM_MINGW    := 20261020
MACOS_SDK_VER := 15.5
MACOS_MIN     := 14.4

CACHE    ?= $(HOME)/.cache/meteobench-static
JOBS     ?= $(shell nproc)
HOST_QT  ?= /usr
CLANG    ?= $(shell command -v clang)
CLANGXX  ?= $(shell command -v clang++)
DIST     := build/dist

QT_URL     := https://download.qt.io/official_releases/qt/$(QT_MINOR)/$(QT_VER)/submodules/qtbase-everywhere-src-$(QT_VER).tar.xz
MINGW_URL  := https://github.com/mstorsjo/llvm-mingw/releases/download/$(LLVM_MINGW)/llvm-mingw-$(LLVM_MINGW)-ucrt-ubuntu-22.04-x86_64.tar.xz
SDK_URL    := https://github.com/joseluisq/macosx-sdks/releases/download/$(MACOS_SDK_VER)/MacOSX$(MACOS_SDK_VER).sdk.tar.xz
UBUNTU_URL := https://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04.5-base-amd64.tar.gz
CMAKE_URL  := https://github.com/Kitware/CMake/releases/download/v3.31.6/cmake-3.31.6-linux-x86_64.tar.gz

QT_SRC     := $(CACHE)/src/qtbase-everywhere-src-$(QT_VER)
MINGW_DIR  := $(CACHE)/toolchains/llvm-mingw-$(LLVM_MINGW)
SDK_DIR    := $(CACHE)/toolchains/MacOSX$(MACOS_SDK_VER).sdk
FAKEBIN    := $(CACHE)/toolchains/fakebin
TC         := $(CACHE)/toolchains/cmake
ROOTFS     := $(CACHE)/rootfs-jammy
# Run a command in the Ubuntu rootfs; the project is at /project, $(CACHE) at /cache.
IN_JAMMY   := scripts/jammy-run $(ROOTFS) "$(CURDIR)" $(CACHE)

JAMMY_PKGS := gcc-12 g++-12 make ninja-build perl python3 pkg-config file binutils \
  libfontconfig1-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev \
  libsm-dev libice-dev libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-icccm4-dev \
  libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-shape0-dev \
  libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev libxcb-xinerama0-dev \
  libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libwayland-dev libdbus-1-dev

# Features we don't need; trims the static Qt builds.
QT_COMMON := -static -release -opensource -confirm-license \
             -nomake examples -nomake tests \
             -no-feature-sql -no-feature-testlib -no-icu
QT_CROSS  := -no-dbus -no-pkg-config -- -DQT_HOST_PATH=$(HOST_QT) -DQT_HOST_PATH_CMAKE_DIR=$(HOST_QT)/lib/cmake

APP_CMAKE := -G Ninja -DCMAKE_BUILD_TYPE=Release

.PHONY: all dist dist-linux dist-windows dist-macos-intel dist-macos-arm distclean FORCE
.NOTPARALLEL:

all:
	cmake -S . -B build
	cmake --build build --parallel $(JOBS)

dist: dist-linux dist-windows dist-macos-intel dist-macos-arm
	@echo
	@echo "Distribution binaries:"
	@ls -lh $(DIST)
	@file $(DIST)/*

dist-linux:       $(DIST)/meteobench_linux
dist-windows:     $(DIST)/meteobench_windows.exe
dist-macos-intel: $(DIST)/meteobench_macos_intel
dist-macos-arm:   $(DIST)/meteobench_macos_apple_silicon

distclean:
	rm -rf $(DIST) build/dist-*

# ---------------------------------------------------------------------------
# Downloads
# ---------------------------------------------------------------------------

$(QT_SRC)/.stamp:
	mkdir -p $(CACHE)/src
	curl -fL --retry 3 -o $(CACHE)/src/qtbase.tar.xz $(QT_URL)
	tar -xf $(CACHE)/src/qtbase.tar.xz -C $(CACHE)/src
	rm -f $(CACHE)/src/qtbase.tar.xz
	touch $@

$(MINGW_DIR)/.stamp:
	mkdir -p $(CACHE)/toolchains
	curl -fL --retry 3 -o $(CACHE)/toolchains/llvm-mingw.tar.xz $(MINGW_URL)
	rm -rf $(MINGW_DIR) && mkdir -p $(MINGW_DIR)
	tar -xf $(CACHE)/toolchains/llvm-mingw.tar.xz -C $(MINGW_DIR) --strip-components=1
	rm -f $(CACHE)/toolchains/llvm-mingw.tar.xz
	touch $@

$(SDK_DIR)/.stamp:
	mkdir -p $(CACHE)/toolchains
	curl -fL --retry 3 -o $(CACHE)/toolchains/macsdk.tar.xz $(SDK_URL)
	rm -rf $(SDK_DIR)
	tar -xf $(CACHE)/toolchains/macsdk.tar.xz -C $(CACHE)/toolchains
	rm -f $(CACHE)/toolchains/macsdk.tar.xz
	touch $@

# Ubuntu 22.04 build root for Linux (glibc 2.35), with Kitware CMake (Qt needs >= 3.25).
$(ROOTFS)/.stamp:
	rm -rf $(ROOTFS) && mkdir -p $(ROOTFS)/opt/cmake
	curl -fL --retry 3 -o $(CACHE)/ubuntu-base.tar.gz $(UBUNTU_URL)
	unshare --map-auto --map-root-user tar -xzf $(CACHE)/ubuntu-base.tar.gz -C $(ROOTFS)
	rm -f $(CACHE)/ubuntu-base.tar.gz
	curl -fL --retry 3 -o $(CACHE)/cmake.tar.gz $(CMAKE_URL)
	tar -xzf $(CACHE)/cmake.tar.gz -C $(ROOTFS)/opt/cmake --strip-components=1
	rm -f $(CACHE)/cmake.tar.gz
	rm -f $(ROOTFS)/etc/resolv.conf && cat /etc/resolv.conf > $(ROOTFS)/etc/resolv.conf
	$(IN_JAMMY) 'set -e; \
	  echo "APT::Sandbox::User \"root\";" > /etc/apt/apt.conf.d/99nosandbox; \
	  sed -i "s/^# deb \(.*universe\)/deb \1/" /etc/apt/sources.list; \
	  apt-get update -qq; \
	  apt-get install -y -qq --no-install-recommends $(JAMMY_PKGS); \
	  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 --slave /usr/bin/g++ g++ /usr/bin/g++-12; \
	  ln -sf gcc /usr/bin/cc; ln -sf g++ /usr/bin/c++'
	touch $@

# ---------------------------------------------------------------------------
# CMake toolchain files (generated)
# ---------------------------------------------------------------------------

$(TC)/windows.cmake: $(MINGW_DIR)/.stamp
	mkdir -p $(TC)
	printf '%s\n' \
	  'set(CMAKE_SYSTEM_NAME Windows)' \
	  'set(CMAKE_SYSTEM_PROCESSOR x86_64)' \
	  'set(CMAKE_C_COMPILER   $(MINGW_DIR)/bin/x86_64-w64-mingw32-clang)' \
	  'set(CMAKE_CXX_COMPILER $(MINGW_DIR)/bin/x86_64-w64-mingw32-clang++)' \
	  'set(CMAKE_RC_COMPILER  $(MINGW_DIR)/bin/x86_64-w64-mingw32-windres)' \
	  'set(CMAKE_AR           $(MINGW_DIR)/bin/llvm-ar)' \
	  'set(CMAKE_RANLIB       $(MINGW_DIR)/bin/llvm-ranlib)' \
	  'set(CMAKE_FIND_ROOT_PATH $(MINGW_DIR)/x86_64-w64-mingw32)' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)' \
	  > $@

# Minimal xcrun/xcodebuild stand-ins: Qt's CMake queries them on Apple targets.
$(FAKEBIN)/xcrun: $(SDK_DIR)/.stamp
	mkdir -p $(FAKEBIN)
	printf '%s\n' \
	  '#!/bin/sh' \
	  'while [ $$# -gt 0 ]; do' \
	  '  case "$$1" in' \
	  '    --show-sdk-path) echo "$(SDK_DIR)"; exit 0;;' \
	  '    --show-sdk-version|--show-sdk-platform-version) echo "$(MACOS_SDK_VER)"; exit 0;;' \
	  '    -f|--find) shift; case "$$1" in' \
	  '        clang|clang++|cc|c++) command -v "$$1";;' \
	  '        ld) command -v ld64.lld;;' \
	  '        *) command -v "llvm-$$1" || command -v "$$1";; esac; exit 0;;' \
	  '    xcodebuild) shift; exec "$(FAKEBIN)/xcodebuild" "$$@";;' \
	  '    --sdk|-sdk) shift;;' \
	  '    -*) ;;' \
	  '    *) tool="$$1"; shift; exec "$$(command -v "llvm-$$tool" || command -v "$$tool")" "$$@";;' \
	  '  esac; shift' \
	  'done' > $@
	printf '%s\n' '#!/bin/sh' 'echo "Xcode 16.4"' 'echo "Build version 16F6"' > $(FAKEBIN)/xcodebuild
	chmod +x $@ $(FAKEBIN)/xcodebuild

# $(1) = arch (x86_64 | arm64)
define mac_toolchain
$(TC)/macos-$(1).cmake: $(SDK_DIR)/.stamp $(FAKEBIN)/xcrun
	mkdir -p $(TC)
	printf '%s\n' \
	  'set(CMAKE_SYSTEM_NAME Darwin)' \
	  'set(CMAKE_SYSTEM_PROCESSOR $(1))' \
	  'set(CMAKE_OSX_ARCHITECTURES $(1))' \
	  'set(CMAKE_OSX_SYSROOT $(SDK_DIR))' \
	  'set(CMAKE_OSX_DEPLOYMENT_TARGET $(MACOS_MIN))' \
	  'set(CMAKE_C_COMPILER   $(CLANG))' \
	  'set(CMAKE_CXX_COMPILER $(CLANGXX))' \
	  'set(CMAKE_OBJC_COMPILER   $(CLANG))' \
	  'set(CMAKE_OBJCXX_COMPILER $(CLANGXX))' \
	  'set(CMAKE_C_COMPILER_TARGET      $(1)-apple-darwin)' \
	  'set(CMAKE_CXX_COMPILER_TARGET    $(1)-apple-darwin)' \
	  'set(CMAKE_OBJC_COMPILER_TARGET   $(1)-apple-darwin)' \
	  'set(CMAKE_OBJCXX_COMPILER_TARGET $(1)-apple-darwin)' \
	  'set(CMAKE_AR      llvm-ar)' \
	  'set(CMAKE_RANLIB  llvm-ranlib)' \
	  'set(CMAKE_INSTALL_NAME_TOOL llvm-install-name-tool)' \
	  'set(CMAKE_LIPO    llvm-lipo)' \
	  'set(CMAKE_OTOOL   llvm-otool)' \
	  'set(CMAKE_CXX_FLAGS_INIT    "-nostdinc++ -isystem $(SDK_DIR)/usr/include/c++/v1")' \
	  'set(CMAKE_OBJCXX_FLAGS_INIT "-nostdinc++ -isystem $(SDK_DIR)/usr/include/c++/v1")' \
	  'set(CMAKE_EXE_LINKER_FLAGS_INIT    "-fuse-ld=lld")' \
	  'set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")' \
	  'set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")' \
	  'set(CMAKE_FIND_ROOT_PATH $(SDK_DIR))' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)' \
	  'set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)' \
	  > $$@
endef
$(eval $(call mac_toolchain,x86_64))
$(eval $(call mac_toolchain,arm64))

# ---------------------------------------------------------------------------
# Static qtbase builds
# ---------------------------------------------------------------------------

# $(1) = name, $(2) = extra configure args, $(3) = extra prerequisites, $(4) = env prefix
define qt_build
$(CACHE)/qt-$(1)/.stamp: $(QT_SRC)/.stamp $(3)
	rm -rf $(CACHE)/qt-build-$(1) && mkdir -p $(CACHE)/qt-build-$(1)
	cd $(CACHE)/qt-build-$(1) && $(4) $(QT_SRC)/configure -prefix $(CACHE)/qt-$(1) $(QT_COMMON) $(2)
	cd $(CACHE)/qt-build-$(1) && $(4) cmake --build . --parallel $(JOBS)
	cd $(CACHE)/qt-build-$(1) && $(4) cmake --install .
	touch $$@
endef

MAC_ENV := PATH="$(FAKEBIN):$(PATH)"

# Linux qtbase is built inside the Ubuntu 22.04 rootfs. Everything that varies
# between distros is bundled or dropped; only libs present on every desktop
# remain dynamic (glibc, X11/xcb, wayland, xkbcommon, fontconfig, freetype).
QT_LINUX := -no-opengl -no-glib -xcb -bundled-xcb-xinput -dbus-runtime -no-cups -no-zstd \
            -no-feature-liburing -qt-libmd4c -qt-libjpeg -qt-libpng -qt-harfbuzz -qt-pcre \
            -qt-zlib -qt-doubleconversion -qt-libb2 -fontconfig -system-freetype

$(CACHE)/qt-linux/.stamp: $(QT_SRC)/.stamp $(ROOTFS)/.stamp
	rm -rf $(CACHE)/qt-build-linux && mkdir -p $(CACHE)/qt-build-linux
	$(IN_JAMMY) 'set -e; cd /cache/qt-build-linux; \
	  /cache/src/qtbase-everywhere-src-$(QT_VER)/configure -prefix /cache/qt-linux $(QT_COMMON) $(QT_LINUX); \
	  cmake --build . --parallel $(JOBS); \
	  cmake --install .'
	touch $@
$(eval $(call qt_build,windows,-xplatform win32-clang-g++ -static-runtime $(QT_CROSS) -DCMAKE_TOOLCHAIN_FILE=$(TC)/windows.cmake,$(TC)/windows.cmake,))
$(eval $(call qt_build,mac-x86_64,-no-framework $(QT_CROSS) -DCMAKE_TOOLCHAIN_FILE=$(TC)/macos-x86_64.cmake,$(TC)/macos-x86_64.cmake,$(MAC_ENV)))
$(eval $(call qt_build,mac-arm64,-no-framework $(QT_CROSS) -DCMAKE_TOOLCHAIN_FILE=$(TC)/macos-arm64.cmake,$(TC)/macos-arm64.cmake,$(MAC_ENV)))

# ---------------------------------------------------------------------------
# Meteobench builds (always re-run; cmake/ninja handle incrementality)
# ---------------------------------------------------------------------------

# $(1) = output name, $(2) = qt dir name, $(3) = cmake args, $(4) = env prefix
define app_build
$(DIST)/$(1): $(CACHE)/qt-$(2)/.stamp FORCE
	$(4) cmake -S . -B build/dist-$(2) $(APP_CMAKE) -DCMAKE_PREFIX_PATH=$(CACHE)/qt-$(2) $(3)
	$(4) cmake --build build/dist-$(2) --parallel $(JOBS)
	mkdir -p $(DIST)
	cp build/dist-$(2)/meteobench$(suffix $(1)) $$@
endef

$(DIST)/meteobench_linux: $(CACHE)/qt-linux/.stamp FORCE
	$(IN_JAMMY) 'set -e; cd /project; \
	  cmake -S . -B build/dist-linux $(APP_CMAKE) -DCMAKE_PREFIX_PATH=/cache/qt-linux \
	    "-DCMAKE_EXE_LINKER_FLAGS=-s -static-libstdc++ -static-libgcc"; \
	  cmake --build build/dist-linux --parallel $(JOBS)'
	mkdir -p $(DIST)
	cp build/dist-linux/meteobench $@
$(eval $(call app_build,meteobench_windows.exe,windows,-DCMAKE_TOOLCHAIN_FILE=$(CACHE)/qt-windows/lib/cmake/Qt6/qt.toolchain.cmake -DCMAKE_EXE_LINKER_FLAGS="-s -static",))
$(eval $(call app_build,meteobench_macos_intel,mac-x86_64,-DCMAKE_TOOLCHAIN_FILE=$(CACHE)/qt-mac-x86_64/lib/cmake/Qt6/qt.toolchain.cmake,$(MAC_ENV)))
$(eval $(call app_build,meteobench_macos_apple_silicon,mac-arm64,-DCMAKE_TOOLCHAIN_FILE=$(CACHE)/qt-mac-arm64/lib/cmake/Qt6/qt.toolchain.cmake,$(MAC_ENV)))

FORCE:
