# Popup tests.
#
# popup-decision is display-free: it only links the pure decision unit and
# needs no GTK, X11 or Wayland. popup-probe is built against GTK and is run by
# popup-runtime.sh on a throwaway Xvfb (popup-x11) or a headless sway
# (popup-wayland). The runtime tests need xvfb, sway and wayland-utils; they
# are skipped when those tools are missing, unless
# -DHAMSTER_REQUIRE_RUNTIME_TESTS=ON is given.

add_executable(test-popup-decision
    test-popup-decision.c
    ${PROJECT_SOURCE_DIR}/panel-plugin/popup-decision.c
)
target_include_directories(test-popup-decision PRIVATE
    ${PROJECT_SOURCE_DIR}/panel-plugin
)

add_test(NAME popup-decision COMMAND test-popup-decision)
set_tests_properties(popup-decision PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# The probe speaks to a real display; no libxfce4panel function is called even
# though view.h is included for HVIEW_CAN_PANEL_POPUP_WINDOW.
add_executable(popup-probe
    popup-probe.c
    ${PROJECT_SOURCE_DIR}/panel-plugin/popup-decision.c
)
target_include_directories(popup-probe PRIVATE
    ${PROJECT_SOURCE_DIR}/panel-plugin
    ${GTK_INCLUDE_DIRS}
)
target_link_directories(popup-probe PRIVATE
    ${GTK_LIBRARY_DIRS}
)
target_link_libraries(popup-probe ${GTK_LIBRARIES})

option(HAMSTER_REQUIRE_RUNTIME_TESTS
    "Fail configuration when the popup runtime test tools are missing" OFF)

find_program(XVFB_EXECUTABLE Xvfb)
if(XVFB_EXECUTABLE)
    add_test(NAME popup-x11
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/popup-runtime.sh x11
            $<TARGET_FILE:popup-probe> ${XVFB_EXECUTABLE}
    )
    set_tests_properties(popup-x11 PROPERTIES
        TIMEOUT 120
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    message(STATUS "popup-x11 test skipped: Xvfb not found (install xvfb)")
endif()

find_program(SWAY_EXECUTABLE sway)
find_program(WAYLAND_INFO_EXECUTABLE wayland-info)
if(SWAY_EXECUTABLE AND WAYLAND_INFO_EXECUTABLE)
    add_test(NAME popup-wayland
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/popup-runtime.sh wayland
            $<TARGET_FILE:popup-probe> ${SWAY_EXECUTABLE} ${WAYLAND_INFO_EXECUTABLE}
    )
    set_tests_properties(popup-wayland PROPERTIES
        TIMEOUT 180
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    message(STATUS "popup-wayland test skipped: sway and/or wayland-info not found (install sway wayland-utils)")
endif()

if(HAMSTER_REQUIRE_RUNTIME_TESTS)
    set(HAMSTER_MISSING_RUNTIME_TOOLS)
    if(NOT XVFB_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "Xvfb (package xvfb)")
    endif()
    if(NOT SWAY_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "sway (package sway)")
    endif()
    if(NOT WAYLAND_INFO_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "wayland-info (package wayland-utils)")
    endif()
    if(HAMSTER_MISSING_RUNTIME_TOOLS)
        string(REPLACE ";" ", " HAMSTER_MISSING_RUNTIME_TOOLS_TEXT
            "${HAMSTER_MISSING_RUNTIME_TOOLS}")
        message(FATAL_ERROR
            "HAMSTER_REQUIRE_RUNTIME_TESTS=ON but these tools are missing: "
            "${HAMSTER_MISSING_RUNTIME_TOOLS_TEXT}")
    endif()
endif()
