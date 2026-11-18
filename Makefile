# minimal makefile to ease cmake usage

CMAKE_BUILD_TYPE 		?= RelWithDebInfo # sane, strip later on
CMAKE_INSTALL_PREFIX 	?= /usr # panel plugins only work here
CMAKE_GENERATOR 		?= $(shell (command -v ninja > /dev/null 2>&1 && echo "Ninja") || echo "Unix Makefiles")

# containerised build, same image CI uses
CONTAINER 				?= $(shell (command -v docker > /dev/null 2>&1 && echo "docker") || (command -v podman > /dev/null 2>&1 && echo "podman"))
IMAGE 					?= xfce4-hamster-plugin-ci:local
BUILD_DIR 				?= build
TEST_BUILD_DIR 			?= build-test

.PHONY: default info configure cmake install uninstall deb rpm image docker test docker-test

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

# Build with "make" first; "sudo make install" must not rebuild as root.
install:
	cmake --install build

uninstall:
	@test -f build/install_manifest.txt || (echo "build/install_manifest.txt not found. Run install first." >&2; exit 1)
	xargs -r rm -vf < build/install_manifest.txt

clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)

# build and run the popup tests with the local toolchain
test:
	rm -rf "$(TEST_BUILD_DIR)"
	cmake -B $(TEST_BUILD_DIR) -G $(CMAKE_GENERATOR) \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)
	cmake --build $(TEST_BUILD_DIR) --parallel
	cd $(TEST_BUILD_DIR) && ctest --output-on-failure

deb: cmake
	cd build && cpack -G DEB

rpm: cmake
	cd build && cpack -G RPM

image:
	@test -n "$(CONTAINER)" || (echo "Neither docker nor podman found." >&2; exit 1)
	$(CONTAINER) build -f .github/ci/Dockerfile -t $(IMAGE) .

# build in the CI container, no local toolchain required; the source tree
# stays untouched, artifacts land in build/
docker: image
	mkdir -p "$(BUILD_DIR)"
	# the translation template is generated and git-ignored, so refresh it
	# before the source tree is mounted read-only
	$(CONTAINER) run --rm \
		--user $(shell id -u):$(shell id -g) \
		-e HOME=/tmp \
		-v "$(CURDIR)":/src -w /src/po \
		$(IMAGE) intltool-update -p -g xfce4-hamster-plugin
	$(CONTAINER) run --rm \
		--user $(shell id -u):$(shell id -g) \
		-e HOME=/tmp \
		-v "$(CURDIR)":/src:ro \
		-v "$(abspath $(BUILD_DIR))":/build \
		-w /build \
		$(IMAGE) sh -c '\
			cmake -S /src -B /build -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
				-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) && \
			cmake --build /build --parallel'

# build and run the popup tests in the CI container; needs no local toolchain
# and no local Xvfb/sway, the image provides them. This includes the live panel
# tests (popup-panel-x11 and popup-panel-wayland), which start a real
# xfce4-panel. The runtime tools are mandatory here
# (-DHAMSTER_REQUIRE_RUNTIME_TESTS=ON), so a missing tool fails the configure
# step instead of skipping a test.
docker-test: image
	rm -rf "$(TEST_BUILD_DIR)"
	mkdir -p "$(TEST_BUILD_DIR)"
	# the translation template is generated and git-ignored, so refresh it
	# before the source tree is mounted read-only
	$(CONTAINER) run --rm \
		--user $(shell id -u):$(shell id -g) \
		-e HOME=/tmp \
		-v "$(CURDIR)":/src -w /src/po \
		$(IMAGE) intltool-update -p -g xfce4-hamster-plugin
	$(CONTAINER) run --rm \
		--user $(shell id -u):$(shell id -g) \
		-e HOME=/tmp \
		-v "$(CURDIR)":/src:ro \
		-v "$(abspath $(TEST_BUILD_DIR))":/build \
		-w /build \
		$(IMAGE) sh -c '\
			cmake -S /src -B /build -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
				-DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) \
				-DHAMSTER_REQUIRE_RUNTIME_TESTS=ON && \
			cmake --build /build --parallel && \
			ctest --output-on-failure'
