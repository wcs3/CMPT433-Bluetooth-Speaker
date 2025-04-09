# Build the app, using the HAL

# Add include paths
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/vosk)

# Add the path to the VOSK library
link_directories(${CMAKE_SOURCE_DIR}/vosk)

# Source files
file(GLOB MY_SOURCES "src/*.c")

# Executable
add_executable(433-bluetooth-speaker ${MY_SOURCES})

# Link with internal HAL libraries
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE hal)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE lcd)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE ui)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE lgpio)

# ALSA support
find_package(ALSA REQUIRED)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE asound)

# VOSK support
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE vosk)

# json-c support
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE json-c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE PkgConfig::deps)

# Set runtime search path for shared libraries (during build and post-build usage)
set_target_properties(433-bluetooth-speaker PROPERTIES
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
  SKIP_BUILD_RPATH FALSE
  BUILD_WITH_INSTALL_RPATH TRUE
)

# Copy executable to final location
add_custom_command(TARGET 433-bluetooth-speaker POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:433-bluetooth-speaker>"
     "~/cmpt433/public/433-project/433-bluetooth-speaker" 
  COMMENT "Copying ARM executable to public NFS directory")

# Copy the VOSK model folder
add_custom_command(TARGET 433-bluetooth-speaker POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/vosk/vosk-model-small-en-us-0.15"
    "~/cmpt433/public/433-project/vosk-model-small-en-us-0.15"
  COMMENT "Copying VOSK model to public NFS directory")

# Copy the VOSK shared library
add_custom_command(TARGET 433-bluetooth-speaker POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_SOURCE_DIR}/vosk/libvosk.so"
    "~/cmpt433/public/433-project/libvosk.so"
  COMMENT "Copying VOSK shared library to public NFS directory")
