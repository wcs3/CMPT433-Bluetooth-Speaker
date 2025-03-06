# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "*.c")
add_executable(rotary-encoder-test1 ${MY_SOURCES})

# Make use of the libraries
target_link_libraries(rotary-encoder-test1 LINK_PRIVATE hal)
target_link_libraries(rotary-encoder-test1 LINK_PRIVATE lcd)
target_link_libraries(rotary-encoder-test1 LINK_PRIVATE lgpio)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET rotary-encoder-test1 POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:rotary-encoder-test1>"
     "~/cmpt433/public/433-project/test/rotary-encoder/rotary-encoder-test1" 
  COMMENT "Copying ARM executable to public NFS directory")
