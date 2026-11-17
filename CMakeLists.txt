cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(xfce4-hamster-plugin
    VERSION 1.18
    DESCRIPTION "A recreation of the gnome hamster shell extension for the xfce4 panel"
    LANGUAGES C
)
set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/projecthamster/xfce4-hamster-plugin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
find_package(Gettext REQUIRED)
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

# the template is git-ignored; track the files it is extracted from so it is
# regenerated when they change instead of silently going stale
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/po/POTFILES.in POTFILES)
set(POT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/po/POTFILES.in)
foreach(POTFILE ${POTFILES})
    if(NOT POTFILE MATCHES "^#")
        list(APPEND POT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${POTFILE})
    endif()
endforeach()

add_custom_command(OUTPUT ${POT}
    COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/po && ${INTLTOOLUPDATE} -p -g ${PROJECT_NAME}
    DEPENDS ${POT_DEPENDS}
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hamster.desktop
    COMMAND ${INTLTOOLMERGE} -d ${PROJECT_SOURCE_DIR}/po ${PROJECT_SOURCE_DIR}/panel-plugin/hamster.desktop.in ${PROJECT_BINARY_DIR}/hamster.desktop
    DEPENDS ${PROJECT_SOURCE_DIR}/panel-plugin/hamster.desktop.in translations
)

add_subdirectory(
    panel-plugin
)

# Merge and compile translations in the build tree so a build never writes
# back into po/ (which also lets the source tree be mounted read-only).
# po/*.po is the single source of truth for the language list.
file(GLOB PO_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/po/*.po)
set(PO_LANGUAGES)
foreach(PO_FILE ${PO_SOURCES})
    get_filename_component(LANG ${PO_FILE} NAME_WE)
    list(APPEND PO_LANGUAGES ${LANG})
endforeach()
set(GMO_FILES)
foreach(LANG ${PO_LANGUAGES})
    set(PO_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/po/${LANG}.po)
    set(PO_BUILD ${CMAKE_CURRENT_BINARY_DIR}/po/${LANG}.po)
    set(GMO_BUILD ${CMAKE_CURRENT_BINARY_DIR}/po/${LANG}.gmo)
    add_custom_command(
        OUTPUT ${PO_BUILD}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/po
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PO_SOURCE} ${PO_BUILD}
        DEPENDS ${PO_SOURCE}
    )
    add_custom_command(
        OUTPUT ${GMO_BUILD}
        COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE} --quiet --update --backup=none ${PO_BUILD} ${POT}
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${GMO_BUILD} ${PO_BUILD}
        DEPENDS ${PO_BUILD} ${POT}
    )
    install(FILES ${GMO_BUILD}
        DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${LANG}/LC_MESSAGES
        RENAME ${PROJECT_NAME}.mo
    )
    list(APPEND GMO_FILES ${GMO_BUILD})
endforeach()

add_custom_target(translations ALL DEPENDS ${GMO_FILES})

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
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_RELEASE ${CPACK_ADD_VERSION})

include(CPack)
