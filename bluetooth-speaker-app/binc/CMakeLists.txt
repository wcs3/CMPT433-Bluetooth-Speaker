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

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0)
target_link_libraries(Binc PkgConfig::deps)

target_include_directories (Binc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-D_DEFAULT_SOURCE)