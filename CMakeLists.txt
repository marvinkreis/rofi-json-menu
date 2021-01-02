cmake_minimum_required(VERSION 2.8)
project(rofi-json-menu)



# Plugin

find_package(PkgConfig)
pkg_search_module(GLIB2 REQUIRED glib-2.0)
pkg_search_module(CAIRO REQUIRED cairo)
pkg_search_module(JSONC REQUIRED json-c)

pkg_get_variable(ROFI_PLUGINS_DIR rofi pluginsdir)

include_directories(
    include
    ${GLIB2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${JSONC_INCLUDE_DIRS}
)

file(GLOB SRC "src/*.c")

add_library(jsonmenu SHARED ${SRC})
set_target_properties(jsonmenu PROPERTIES PREFIX "")

target_link_libraries(jsonmenu
    ${GLIB2_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${JSONC_LIBRARIES}
)

install(TARGETS jsonmenu DESTINATION ${ROFI_PLUGINS_DIR})
