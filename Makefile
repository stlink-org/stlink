##
# This Makefile is used to drive building of CMake build targets
##

MAKEFLAGS += -s

# additional flags for cmake, e.g. install path -DCMAKE_INSTALL_PREFIX=$(HOME)/.local
CMAKEFLAGS +=

all: release
ci: debug release test

help:
		@echo "        debug: Run a debug build"
		@echo "      release: Run a release build"
		@echo "      install: Install release build"
		@echo "    uninstall: Uninstall release build"
		@echo "      package: Package release build"
		@echo "         lint: Lint check all source-code"
		@echo "         test: Build and run tests"
		@echo "        clean: Clean all build output"
		@echo "rebuild_cache: Rebuild all CMake caches"

rebuild_cache: build/Debug build/Release
		@$(MAKE) -C build/Debug rebuild_cache
		@$(MAKE) -C build/Release rebuild_cache

debug: build/Debug
		@echo "[DEBUG]"
		@$(MAKE) -C build/Debug

release: build/Release
		@echo "[RELEASE]"
		@$(MAKE) -C build/Release

install: build/Release
		@echo "[INSTALL] Release"
		@$(MAKE) -C build/Release install

uninstall: build/Release
		@echo "[UNINSTALL] Release"
		@$(MAKE) -C build/Release uninstall

package: build/Release
		@echo "[PACKAGE] Release"
		@$(MAKE) -C build/Release package

test: debug
		@echo "[TEST] Debug"
		@$(MAKE) -C build/Debug test

build/Debug:
		@mkdir -p $@
		@cd $@ && cmake -DCMAKE_BUILD_TYPE=Debug $(CMAKEFLAGS) ../../

build/Release:
		@mkdir -p $@
		@cd $@ && cmake -DCMAKE_BUILD_TYPE=Release $(CMAKEFLAGS) ../../

clean:
		@echo "[CLEAN]"
		@rm -Rf build

.PHONY: clean
