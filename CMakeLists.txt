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

# place executables next to this CMakeLists.txt
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# Build executable (add stb_image.cpp to pull in implementation)
add_executable(MyJPEGApp
    src/main.cpp
    src/stb_image.cpp
)

# Link library (choose jpeg-static if using static version)
target_link_libraries(MyJPEGApp jpeg-static)