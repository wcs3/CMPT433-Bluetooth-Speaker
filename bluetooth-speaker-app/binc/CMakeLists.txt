add_library(Binc
        adapter.c
        advertisement.c
        agent.c
        application.c
        characteristic.c
        descriptor.c
        device.c
        logger.c
        parser.c
        service.c
        utility.c
        )

get_target_property(target_options Binc COMPILE_OPTIONS)
list(REMOVE_ITEM target_options "-Werror")
list(REMOVE_ITEM target_options "-Wextra")
list(REMOVE_ITEM target_options "-Wpedantic")
set_property(TARGET Binc PROPERTY COMPILE_OPTIONS ${target_options})

include(FindPkgConfig)
pkg_check_modules(GLIB glib-2.0 gio-2.0 REQUIRED)
include_directories(${GLIB_INCLUDE_DIRS})

target_include_directories (Binc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Binc ${GLIB_LIBRARIES} m)

add_definitions(-D_DEFAULT_SOURCE)