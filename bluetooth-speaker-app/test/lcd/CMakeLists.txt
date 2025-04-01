# Build the app, using the HAL

include_directories(include)
# file(GLOB MY_SOURCES "*.c")
add_executable(lcd-test1 "lcd-test1.c")
add_executable(lcd-test2 "lcd-test2.c")

# Make use of the libraries
target_link_libraries(lcd-test1 LINK_PRIVATE hal)
target_link_libraries(lcd-test1 LINK_PRIVATE lcd)
target_link_libraries(lcd-test1 LINK_PRIVATE lgpio)
target_link_libraries(lcd-test1 LINK_PRIVATE ui)

target_link_libraries(lcd-test2 LINK_PRIVATE hal)
target_link_libraries(lcd-test2 LINK_PRIVATE lcd)
target_link_libraries(lcd-test2 LINK_PRIVATE lgpio)
target_link_libraries(lcd-test2 LINK_PRIVATE ui)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET lcd-test1 POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:lcd-test1>"
     "~/cmpt433/public/433-project/test/lcd/lcd-test1" 
  COMMENT "Copying ARM executable to public NFS directory")

add_custom_command(TARGET lcd-test2 POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:lcd-test2>"
     "~/cmpt433/public/433-project/test/lcd/lcd-test2" 
  COMMENT "Copying ARM executable to public NFS directory")
