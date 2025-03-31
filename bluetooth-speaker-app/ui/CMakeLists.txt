

include_directories(ui/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(ui STATIC ${MY_SOURCES})

target_include_directories(ui PUBLIC include)

# Link lcd, lgpio to hal
target_link_libraries(ui PUBLIC hal)