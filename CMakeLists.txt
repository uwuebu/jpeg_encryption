cmake_minimum_required(VERSION 3.16)
project(Dyplomna)

set(CMAKE_CXX_STANDARD 17)

# Allow user to define path to JPEG externally or default to common dir
set(JPEG_TURBO_DIR "C:/libraries/libjpeg-turbo64" CACHE PATH "Path to libjpeg-turbo")

# Include dirs
include_directories("${JPEG_TURBO_DIR}/include")
link_directories("${JPEG_TURBO_DIR}/lib")
# Add project headers (stb_image)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# Build executable (add stb_image.cpp to pull in implementation)
add_executable(MyJPEGApp
    src/main.cpp
    src/stb_image.cpp
    src/jpeg.cpp          # added JPEG class implementation
    src/chaotic_keystream_generator.cpp # Add this line
)

# Link library (choose jpeg-static if using static version)
target_link_libraries(MyJPEGApp jpeg-static)