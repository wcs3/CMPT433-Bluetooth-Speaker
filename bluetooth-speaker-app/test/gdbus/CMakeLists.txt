add_executable(gdbus_test dbus.c player.c)

get_target_property(target_options gdbus_test COMPILE_OPTIONS)
# list(REMOVE_ITEM target_options "-Werror")
# list(REMOVE_ITEM target_options "-Wextra")
list(REMOVE_ITEM target_options "-Wpedantic")
set_property(TARGET gdbus_test PROPERTY COMPILE_OPTIONS ${target_options})

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0)
target_link_libraries(gdbus_test PkgConfig::deps)

add_custom_command(TARGET gdbus_test POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:gdbus_test>"
     "~/cmpt433/public/433-project/test/gdbus/gbus_test" 
  COMMENT "Copying ARM executable to public NFS directory") 