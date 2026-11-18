# Popup tests.
#
# popup-decision is display-free: it only links the pure decision unit and
# needs no GTK, X11 or Wayland. popup-probe is built against GTK and is run by
# popup-runtime.sh on a throwaway Xvfb (popup-x11) or a headless sway
# (popup-wayland). popup-panel-x11 and popup-panel-wayland go one step
# further: popup-panel.sh starts a real xfce4-panel with the built plugin and
# asserts the popup through the real plugin event path and button clicks. They
# need the panel, a window manager or compositor, and screenshot/pointer
# helpers; they are skipped when those tools are missing, unless
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

# Screenshot comparison for the live Wayland test. It only needs
# gdk-pixbuf, which GTK already pulls in, so it is always built.
add_executable(popup-image-diff popup-image-diff.c)
target_include_directories(popup-image-diff PRIVATE
    ${GTK_INCLUDE_DIRS}
)
target_link_directories(popup-image-diff PRIVATE
    ${GTK_LIBRARY_DIRS}
)
target_link_libraries(popup-image-diff ${GTK_LIBRARIES})

find_program(XFCE4_PANEL_EXECUTABLE xfce4-panel)
find_program(XFWM4_EXECUTABLE xfwm4)
find_program(XDOTOOL_EXECUTABLE xdotool)
find_program(XWININFO_EXECUTABLE xwininfo)
find_program(XPROP_EXECUTABLE xprop)
find_program(XMESSAGE_EXECUTABLE xmessage)
find_program(DBUS_DAEMON_EXECUTABLE dbus-daemon)
find_program(XFCONF_QUERY_EXECUTABLE xfconf-query)
find_program(GRIM_EXECUTABLE grim)
find_program(SWAYMSG_EXECUTABLE swaymsg)

# The Wayland click helper is a standalone wayland-client program speaking the
# vendored wlr virtual-pointer protocol; wayland-scanner generates the glue at
# build time.
pkg_check_modules(WAYLAND_CLIENT wayland-client)
find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner)
if(WAYLAND_SCANNER_EXECUTABLE AND WAYLAND_CLIENT_FOUND)
    set(WLR_VIRTUAL_POINTER_XML
        ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-virtual-pointer-unstable-v1.xml)
    set(WLR_VIRTUAL_POINTER_HEADER
        ${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-client-protocol.h)
    set(WLR_VIRTUAL_POINTER_CODE
        ${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-protocol.c)
    add_custom_command(
        OUTPUT
            ${WLR_VIRTUAL_POINTER_HEADER}
            ${WLR_VIRTUAL_POINTER_CODE}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            ${WLR_VIRTUAL_POINTER_XML} ${WLR_VIRTUAL_POINTER_HEADER}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code
            ${WLR_VIRTUAL_POINTER_XML} ${WLR_VIRTUAL_POINTER_CODE}
        DEPENDS ${WLR_VIRTUAL_POINTER_XML}
        VERBATIM
    )
    # The generated header is listed as a source so it exists before the
    # compiler opens it; the include dir makes the generated name visible.
    add_executable(popup-wayland-input
        popup-wayland-input.c
        ${WLR_VIRTUAL_POINTER_CODE}
        ${WLR_VIRTUAL_POINTER_HEADER}
    )
    target_include_directories(popup-wayland-input PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
    )
    target_link_directories(popup-wayland-input PRIVATE
        ${WAYLAND_CLIENT_LIBRARY_DIRS}
    )
    target_link_libraries(popup-wayland-input ${WAYLAND_CLIENT_LIBRARIES})
endif()

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

# Every argument must be a resolved program before add_test is reached: CMake
# stores "NOTFOUND" as a literal test argument otherwise.
set(HAMSTER_PANEL_X11_MISSING)
if(NOT XVFB_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "Xvfb (package xvfb)")
endif()
if(NOT XFCE4_PANEL_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "xfce4-panel (package xfce4-panel)")
endif()
if(NOT XFWM4_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "xfwm4 (package xfwm4)")
endif()
if(NOT XDOTOOL_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "xdotool (package xdotool)")
endif()
if(NOT XWININFO_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "xwininfo (package x11-utils)")
endif()
if(NOT XPROP_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "xprop (package x11-utils)")
endif()
if(NOT XMESSAGE_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "xmessage (package x11-utils)")
endif()
if(NOT DBUS_DAEMON_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "dbus-daemon (package dbus-daemon)")
endif()
if(NOT XFCONF_QUERY_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_X11_MISSING "xfconf-query (package xfconf)")
endif()
if(HAMSTER_PANEL_X11_MISSING)
    string(REPLACE ";" ", " HAMSTER_PANEL_X11_MISSING_TEXT
        "${HAMSTER_PANEL_X11_MISSING}")
    message(STATUS "popup-panel-x11 test skipped: missing ${HAMSTER_PANEL_X11_MISSING_TEXT}")
else()
    add_test(NAME popup-panel-x11
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/popup-panel.sh x11
            $<TARGET_FILE:hamster>
            ${PROJECT_BINARY_DIR}/hamster.desktop
            ${CMAKE_INSTALL_PREFIX}
            ${CMAKE_INSTALL_LIBDIR}
            ${CMAKE_INSTALL_DATAROOTDIR}
            ${PROJECT_BINARY_DIR}/xfce4-popup-hamstermenu
            ${XVFB_EXECUTABLE}
            ${XFWM4_EXECUTABLE}
            ${XDOTOOL_EXECUTABLE}
            ${XWININFO_EXECUTABLE}
            ${XPROP_EXECUTABLE}
            ${XMESSAGE_EXECUTABLE}
            ${DBUS_DAEMON_EXECUTABLE}
            ${XFCONF_QUERY_EXECUTABLE}
    )
    set_tests_properties(popup-panel-x11 PROPERTIES
        TIMEOUT 240
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

set(HAMSTER_PANEL_WAYLAND_MISSING)
if(NOT SWAY_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "sway (package sway)")
endif()
if(NOT SWAYMSG_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "swaymsg (package sway)")
endif()
if(NOT GRIM_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "grim (package grim)")
endif()
if(NOT XFCE4_PANEL_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "xfce4-panel (package xfce4-panel)")
endif()
if(NOT DBUS_DAEMON_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "dbus-daemon (package dbus-daemon)")
endif()
if(NOT XFCONF_QUERY_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "xfconf-query (package xfconf)")
endif()
if(NOT WAYLAND_SCANNER_EXECUTABLE)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "wayland-scanner (package libwayland-bin)")
endif()
if(NOT WAYLAND_CLIENT_FOUND)
    list(APPEND HAMSTER_PANEL_WAYLAND_MISSING "wayland-client (package libwayland-dev)")
endif()
if(HAMSTER_PANEL_WAYLAND_MISSING)
    string(REPLACE ";" ", " HAMSTER_PANEL_WAYLAND_MISSING_TEXT
        "${HAMSTER_PANEL_WAYLAND_MISSING}")
    message(STATUS "popup-panel-wayland test skipped: missing ${HAMSTER_PANEL_WAYLAND_MISSING_TEXT}")
else()
    add_test(NAME popup-panel-wayland
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/popup-panel.sh wayland
            $<TARGET_FILE:hamster>
            ${PROJECT_BINARY_DIR}/hamster.desktop
            ${CMAKE_INSTALL_PREFIX}
            ${CMAKE_INSTALL_LIBDIR}
            ${CMAKE_INSTALL_DATAROOTDIR}
            ${PROJECT_BINARY_DIR}/xfce4-popup-hamstermenu
            ${SWAY_EXECUTABLE}
            ${SWAYMSG_EXECUTABLE}
            ${GRIM_EXECUTABLE}
            $<TARGET_FILE:popup-wayland-input>
            $<TARGET_FILE:popup-image-diff>
            ${DBUS_DAEMON_EXECUTABLE}
            ${XFCONF_QUERY_EXECUTABLE}
    )
    set_tests_properties(popup-panel-wayland PROPERTIES
        TIMEOUT 240
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
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
    if(NOT XFCE4_PANEL_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "xfce4-panel (package xfce4-panel)")
    endif()
    if(NOT XFWM4_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "xfwm4 (package xfwm4)")
    endif()
    if(NOT XDOTOOL_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "xdotool (package xdotool)")
    endif()
    if(NOT XWININFO_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "xwininfo (package x11-utils)")
    endif()
    if(NOT XPROP_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "xprop (package x11-utils)")
    endif()
    if(NOT XMESSAGE_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "xmessage (package x11-utils)")
    endif()
    if(NOT DBUS_DAEMON_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "dbus-daemon (package dbus-daemon)")
    endif()
    if(NOT XFCONF_QUERY_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "xfconf-query (package xfconf)")
    endif()
    if(NOT GRIM_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "grim (package grim)")
    endif()
    if(NOT SWAYMSG_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "swaymsg (package sway)")
    endif()
    if(NOT WAYLAND_SCANNER_EXECUTABLE)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "wayland-scanner (package libwayland-bin)")
    endif()
    if(NOT WAYLAND_CLIENT_FOUND)
        list(APPEND HAMSTER_MISSING_RUNTIME_TOOLS "wayland-client (package libwayland-dev)")
    endif()
    if(HAMSTER_MISSING_RUNTIME_TOOLS)
        string(REPLACE ";" ", " HAMSTER_MISSING_RUNTIME_TOOLS_TEXT
            "${HAMSTER_MISSING_RUNTIME_TOOLS}")
        message(FATAL_ERROR
            "HAMSTER_REQUIRE_RUNTIME_TESTS=ON but these tools are missing: "
            "${HAMSTER_MISSING_RUNTIME_TOOLS_TEXT}")
    endif()
endif()
