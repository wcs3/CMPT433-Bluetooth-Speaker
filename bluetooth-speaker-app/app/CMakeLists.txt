# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(433-bluetooth-speaker ${MY_SOURCES})

# Make use of the libraries
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE hal)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE lcd)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE ui)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE lgpio)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE Binc)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET 433-bluetooth-speaker POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:433-bluetooth-speaker>"
     "~/cmpt433/public/433-project/433-bluetooth-speaker" 
  COMMENT "Copying ARM executable to public NFS directory")
