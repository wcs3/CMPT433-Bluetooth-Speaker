# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

target_include_directories(hal PUBLIC include)

# Link lcd, lgpio to hal
target_link_libraries(hal PUBLIC lgpio)
target_link_libraries(hal PUBLIC lcd)
# link libm because stb_image needs it
target_link_libraries(hal PUBLIC m)
# Support GPIO
find_library(GPIOD_LIBRARY gpiod)       # UNSURE IF NEEDED
target_link_libraries(hal LINK_PRIVATE gpiod) # May need to change to HAL lib.