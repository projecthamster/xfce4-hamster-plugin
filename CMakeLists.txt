cmake_minimum_required(VERSION 3.13)

project(xfce4-hamster-plugin
    VERSION 1.18
    DESCRIPTION "A recreation of the gnome hamster shell extension for the xfce4 panel"
    LANGUAGES C
)

find_package(PkgConfig REQUIRED)
find_package(Gettext REQUIRED)
find_package(Intl REQUIRED)
include(GNUInstallDirs)

pkg_check_modules(GTK REQUIRED
    libxfce4panel-2.0
    libxfce4ui-2
    libxfconf-0
)

find_program(GDBUSCODEGEN NAMES gdbus-codegen)
if (NOT GDBUSCODEGEN)
    message(SEND_ERROR "Could not find gdbus-codegen")
endif()

find_program(INTLTOOLUPDATE NAMES intltool-update)
if (NOT INTLTOOLUPDATE)
    message(SEND_ERROR "Could not find intltool-update")
endif()

find_program(INTLTOOLMERGE NAMES intltool-merge)
if (NOT INTLTOOLMERGE)
    message(SEND_ERROR "Could not find intltool-merge")
endif()

file(GLOB pos
    LIST_DIRECTORIES false
    ${CMAKE_CURRENT_LIST_DIR}/po/*.po
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/po/${PROJECT_NAME}.pot
    COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/po && ${INTLTOOLUPDATE} -p -g ${PROJECT_NAME}
    DEPENDS ${pos}
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hamster.desktop
    COMMAND ${INTLTOOLMERGE} -d ${PROJECT_SOURCE_DIR}/po ${PROJECT_SOURCE_DIR}/panel-plugin/hamster.desktop.in ${PROJECT_BINARY_DIR}/hamster.desktop
    DEPENDS ${PROJECT_SOURCE_DIR}/panel-plugin/hamster.desktop.in
)

add_subdirectory(
    panel-plugin
)

gettext_create_translations(
    po/${PROJECT_NAME}.pot ALL
    po/cs.po
    po/de.po
    po/es.po
    po/ru.po
    po/tr.po
    po/uk.po
)

configure_file(
    ${PROJECT_SOURCE_DIR}/panel-plugin/xfce4-popup-hamstermenu.sh
    ${PROJECT_BINARY_DIR}/xfce4-popup-hamstermenu
)

add_custom_target(
    data ALL
    DEPENDS
    ${PROJECT_BINARY_DIR}/hamster.desktop
    ${PROJECT_SOURCE_DIR}/po/${PROJECT_NAME}.pot
    ${PROJECT_BINARY_DIR}/xfce4-popup-hamstermenu
)

install(TARGETS hamster  LIBRARY DESTINATION lib/xfce4/panel/plugins/)
install(FILES ${PROJECT_BINARY_DIR}/hamster.desktop DESTINATION share/xfce4/panel/plugins/)
install(PROGRAMS ${PROJECT_BINARY_DIR}/xfce4-popup-hamstermenu DESTINATION bin)

