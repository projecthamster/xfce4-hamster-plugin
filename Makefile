# minimal makefile to ease cmake usage

CMAKE_BUILD_TYPE 		?= RelWithDebInfo # sane, strip later on
CMAKE_INSTALL_PREFIX 	?= /usr # panel plugins only work here
CMAKE_GENERATOR 		?= $(shell (command -v ninja > /dev/null 2>&1 && echo "Ninja") || echo "Unix Makefiles")
CMAKE_MAKEFILE 			= $(shell test "$(CMAKE_GENERATOR)" = "Ninja" && echo "Makefile" || echo "build.ninja" )
default: cmake

./build:
	cmake -B build -G  $(CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)

cmake: ./build
	cmake --build build 

install: cmake
	cmake --install build

clean:
	rm -rf build

deb: cmake
	cd build && cpack -G DEB

rpm: cmake
	cd build && cpack -G RPM
