cmake_minimum_required(VERSION 3.13)

project(xfce4-hamster-plugin
    VERSION 1.18
    DESCRIPTION "A recreation of the gnome hamster shell extension for the xfce4 panel"
    LANGUAGES C
)
set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/projecthamster/xfce4-hamster-plugin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(POT ${CMAKE_CURRENT_LIST_DIR}/po/${PROJECT_NAME}.pot)

add_custom_command(OUTPUT ${POT}
    COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/po && ${INTLTOOLUPDATE} -p -g ${PROJECT_NAME}
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hamster.desktop
    COMMAND ${INTLTOOLMERGE} -d ${PROJECT_SOURCE_DIR}/po ${PROJECT_SOURCE_DIR}/panel-plugin/hamster.desktop.in ${PROJECT_BINARY_DIR}/hamster.desktop
    DEPENDS ${PROJECT_SOURCE_DIR}/panel-plugin/hamster.desktop.in translations
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
    ${POT}
    ${PROJECT_BINARY_DIR}/hamster.desktop
    ${PROJECT_BINARY_DIR}/xfce4-popup-hamstermenu
)

install(TARGETS hamster  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/xfce4/panel/plugins/)
install(FILES ${PROJECT_BINARY_DIR}/hamster.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/xfce4/panel/plugins/)
install(PROGRAMS ${PROJECT_BINARY_DIR}/xfce4-popup-hamstermenu DESTINATION ${CMAKE_INSTALL_BINDIR})

# CPACK
set(CPACK_ADD_VERSION 2)
set(CPACK_PACKAGE_CONTACT "Hakan Erduman <hakan@erduman.de>")

# deb specific
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_RELEASE ${CPACK_ADD_VERSION})
set(CPACK_DEBIAN_PACKAGE_SECTION "Xfce")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# rpm specific
set(CPACK_RPM_FILE_NAME, "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_RELEASE ${CPACK_ADD_VERSION})

include(CPack)
