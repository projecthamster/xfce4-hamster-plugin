
add_custom_command(OUTPUT hamster.c hamster.h
    COMMAND ${GDBUSCODEGEN} --generate-c-code hamster --interface-prefix org.gnome. ${CMAKE_CURRENT_LIST_DIR}/org.gnome.Hamster.xml
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/org.gnome.Hamster.xml
)

add_custom_command(OUTPUT windowserver.c windowserver.h
    COMMAND ${GDBUSCODEGEN} --generate-c-code windowserver --interface-prefix org.gnome.Hamster ${CMAKE_CURRENT_LIST_DIR}/org.gnome.Hamster.WindowServer.xml
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/org.gnome.Hamster.WindowServer.xml
)

add_custom_command(OUTPUT xfce4-hamster-plugin.desktop
    COMMAND ${GDBUSCODEGEN} --generate-c-code windowserver --interface-prefix org.gnome.Hamster ${CMAKE_CURRENT_LIST_DIR}/org.gnome.Hamster.WindowServer.xml
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/org.gnome.Hamster.WindowServer.xml
)

add_library(
    hamster SHARED
    button.c        button.h
    hamster.c       hamster.h
    plugin.c
    settings.c      settings.h
    util.c          util.h
    view.c          view.h
    windowserver.c  windowserver.h
)

add_dependencies(
    hamster
    data
)

add_compile_definitions(
    GETTEXT_PACKAGE="${PROJECT_NAME}"
    LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
)

if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    message("Build DEBUG version")
    add_compile_definitions(DEBUG=1)
endif()

target_include_directories(
    hamster PUBLIC
    ${GTK_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/panel-plugin
)

target_link_directories(
    hamster PUBLIC
    ${GTK_LIBRARY_DIRS}
)

target_link_libraries(
    hamster ${GTK_LIBRARIES}
)
