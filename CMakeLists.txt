cmake_minimum_required(VERSION 3.10)
project(SimpleVelopackExample)

set(CMAKE_CXX_STANDARD 17)

include_directories(velopack/include)

add_executable(main main.cpp)

# Adjust the library path and file depending on your OS
# Example for Windows:
target_link_libraries(main PRIVATE ${CMAKE_SOURCE_DIR}/velopack/lib/velopack_libc_win_x64_msvc.dll.lib)

# Example for Linux:
# target_link_libraries(main PRIVATE ${CMAKE_SOURCE_DIR}/velopack/lib/libVelopack.a)
