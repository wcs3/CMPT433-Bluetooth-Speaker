# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c" "src/*.cpp")

# Add microphone.cpp manually since it's outside of app/src
list(APPEND MY_SOURCES "${CMAKE_SOURCE_DIR}/hal/src/microphone.cpp")

add_executable(433-bluetooth-speaker ${MY_SOURCES})

# Make use of the libraries
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE hal)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE lcd)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE lgpio)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE Binc)

# ALSA support
find_package(ALSA REQUIRED)
target_link_libraries(433-bluetooth-speaker LINK_PRIVATE asound)

# Add whisper.cpp static library
add_library(whisper STATIC IMPORTED)
set_target_properties(whisper PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../../whisper.cpp/build/src/libwhisper.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../../whisper.cpp"
)

# Add ggml static library
add_library(ggml STATIC IMPORTED)
set_target_properties(ggml PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../../whisper.cpp/build/ggml/src/libggml.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../../whisper.cpp/ggml/include"
)

# Link additional GGML static libs directly
target_link_libraries(433-bluetooth-speaker PRIVATE
    "${CMAKE_SOURCE_DIR}/../../whisper.cpp/build/ggml/src/libggml.a"
    "${CMAKE_SOURCE_DIR}/../../whisper.cpp/build/ggml/src/libggml-base.a"
    "${CMAKE_SOURCE_DIR}/../../whisper.cpp/build/ggml/src/libggml-cpu.a"
   #  "${CMAKE_SOURCE_DIR}/../../whisper.cpp/build/ggml/src/libggml-backend.a"
    whisper
)

# Copy executable to final location
add_custom_command(TARGET 433-bluetooth-speaker POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:433-bluetooth-speaker>"
     "~/cmpt433/public/433-project/433-bluetooth-speaker" 
  COMMENT "Copying ARM executable to public NFS directory")

# Copy Whisper model to NFS folder
add_custom_command(TARGET 433-bluetooth-speaker POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_SOURCE_DIR}/../../whisper.cpp/models/ggml-base.en.bin"
    "$ENV{HOME}/cmpt433/public/433-project/models/ggml-base.en.bin"
  COMMENT "Copying Whisper model to public NFS directory"
)

