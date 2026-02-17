# minimal makefile to ease cmake usage

CMAKE_BUILD_TYPE 		?= RelWithDebInfo # sane, strip later on
CMAKE_INSTALL_PREFIX 	?= /usr # panel plugins only work here
CMAKE_GENERATOR 		?= $(shell (command -v ninja > /dev/null 2>&1 && echo "Ninja") || echo "Unix Makefiles")

.PHONY: default info configure cmake install uninstall deb rpm

default: cmake

info:
	@echo '*** you can override these ***'
	@echo "CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)"
	@echo "CMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)"
	@echo "CMAKE_GENERATOR=$(CMAKE_GENERATOR)"
	@echo '*** this one depends on CMAKE_GENERATOR ***'

configure:
	cmake -B build -G  $(CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)

cmake: configure
	cmake --build build 

install: cmake
	cmake --install build

uninstall:
	@test -f build/install_manifest.txt || (echo "build/install_manifest.txt not found. Run install first." >&2; exit 1)
	xargs -r rm -vf < build/install_manifest.txt

clean:
	rm -rf build

deb: cmake
	cd build && cpack -G DEB

rpm: cmake
	cd build && cpack -G RPM
