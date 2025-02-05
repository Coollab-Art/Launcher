cmake_minimum_required(VERSION 3.20)
project(Coollab-Launcher)
add_executable(${PROJECT_NAME})

add_library(Coollab-Launcher-Properties INTERFACE) # This is needed by both the launcher and its tests
target_link_libraries(${PROJECT_NAME} PRIVATE Coollab-Launcher-Properties)

target_compile_features(Coollab-Launcher-Properties INTERFACE cxx_std_20)

# Set the folder where the executable is created
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# Set warning level
if(MSVC)
    target_compile_options(Coollab-Launcher-Properties INTERFACE /W4)
else()
    target_compile_options(Coollab-Launcher-Properties INTERFACE -Wall -Wextra -Wpedantic -pedantic-errors -Wconversion -Wsign-conversion -Wimplicit-fallthrough)
endif()

# Maybe enable warnings as errors
set(WARNINGS_AS_ERRORS_FOR_COOLLAB_LAUNCHER OFF CACHE BOOL "ON iff you want to treat warnings as errors") # Might be overridden in the CMake cache

if(WARNINGS_AS_ERRORS_FOR_COOLLAB_LAUNCHER)
    if(MSVC)
        target_compile_options(Coollab-Launcher-Properties INTERFACE /WX)
    else()
        target_compile_options(Coollab-Launcher-Properties INTERFACE -Werror)
    endif()
endif()

include(FetchContent)

# TODO(Launcher) don't copy ffmpeg DLLs, they are not needed for the launcher

# Make sure we link the static version
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

# spawn_process
add_subdirectory(lib/spawn_process)
target_link_libraries(Coollab-Launcher-Properties INTERFACE Cool::spawn_process)

# JSON
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)
target_link_libraries(Coollab-Launcher-Properties INTERFACE nlohmann_json::nlohmann_json)

# MINIZ
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.0.2
)
FetchContent_MakeAvailable(miniz)
target_link_libraries(Coollab-Launcher-Properties INTERFACE miniz)

# Grab all the source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(Coollab-Launcher-Properties INTERFACE ${PROJECT_SOURCE_DIR}/src)

# Embed the manifest file into the executable. This manifest tells windows that our app supports long paths
if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Coollab-Launcher.manifest")
endif()

# OpenSSL
set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(OpenSSL 3)

if(NOT OpenSSL_FOUND)
    find_package(OpenSSL 1.1.1 REQUIRED)
endif()

target_link_libraries(Coollab-Launcher-Properties INTERFACE OpenSSL::SSL OpenSSL::Crypto)

# cpp-httplib
set(HTTPLIB_REQUIRE_OPENSSL ON CACHE BOOL "" FORCE)
add_subdirectory(Lab/lib/cpp-httplib)
target_link_libraries(Coollab-Launcher-Properties INTERFACE httplib)

# Include Cool
set(COOL_USE_OPENGL 410)
set(COOL_USE_INSTALLER ON)
set(COOL_APP_NAME "Coollab Launcher")
add_subdirectory(Lab/Cool)
target_link_libraries(Coollab-Launcher-Properties INTERFACE Cool::Core)
cool_setup(${PROJECT_NAME})
Cool__set_app_icon(${PROJECT_NAME} "Lab/res/logo.png" "${CMAKE_SOURCE_DIR}/Lab/app-resources/icon.rc")
Cool__target_copy_file(${PROJECT_NAME} "Lab/res/logo.png")
install(FILES "Lab/res/logo.png" DESTINATION "Lab/res")

function(Lab_copy_folder FOLDER_RELATIVE_PATH)
    Cool__target_copy_folder(${PROJECT_NAME} ${FOLDER_RELATIVE_PATH})
    install(DIRECTORY ${FOLDER_RELATIVE_PATH} DESTINATION bin)
endfunction()

Lab_copy_folder("User data Default")
Lab_copy_folder("User data Init")

# ---------------------
# ---CPack---
# ---------------------
if(APPLE)
    install(TARGETS ${PROJECT_NAME} DESTINATION ../MacOS) # MacOS bundles must be installed at the root of the package: https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators#dragndrop-osx-only
else()
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
endif()

if(WIN32)
    set(SEP "\\\\")
else()
    set(SEP "/")
endif()

# Installer Info
set(CPACK_PACKAGE_NAME "Coollab Launcher")
set(CPACK_PACKAGE_VERSION "1.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Coollab Launcher") # Don't put this in a Coollab parent folder. In NSIS this messes up the install and prevents our app to appear in "Apps and Features" to uninstall it for example.
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}${SEP}Lab${SEP}app-resources${SEP}installer_banner.bmp") # Seems like it has to be .bmp, at least for NSIS. NB: if there is any transparency in the image, NSIS won't be able to render it at all. Exporting a fully opaque .bmp from Paint.net works. 150x57 pixels is the ideal size to avoid uggly downscale artifacts.
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}${SEP}Lab${SEP}LICENSE.txt")
set(CPACK_PACKAGE_VENDOR "Coollab")
set(CPACK_PACKAGE_EXECUTABLES "Coollab-Launcher" "Coollab Launcher") # Creates Start Menu shortcuts
set(CPACK_PACKAGE_CONTACT "coollab.lib@gmail.com")

# TODO(Launcher) Quand on désinsrtalle le launchze, déqinstaller toutes les vrsions de coollab

# Name of the installer exe file
if(WIN32)
    set(OS_NAME "Windows")
elseif(APPLE)
    set(OS_NAME "MacOS")
else()
    set(OS_NAME "Linux")
endif()

set(CPACK_PACKAGE_FILE_NAME "Coollab-Launcher-${OS_NAME}")

# NSIS specifics
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\Coollab-Launcher.exe")

# .coollab extension
# --- Windows
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    WriteRegStr HKCR '.coollab' '' 'CoollabFile'
    WriteRegStr HKCR 'CoollabFile' '' 'Coollab File'
    WriteRegStr HKCR 'CoollabFile\\\\shell' '' 'open'
    WriteRegStr HKCR 'CoollabFile\\\\DefaultIcon' \\\\
    '' '$INSTDIR\\\\bin\\\\Coollab-Launcher.exe,0'
    WriteRegStr HKCR 'CoollabFile\\\\shell\\\\open\\\\command' \\\\
    '' '$INSTDIR\\\\bin\\\\Coollab-Launcher.exe \\\"%1\\\"'
    WriteRegStr HKCR 'CoollabFile\\\\shell\\\\edit\\\\command' \\\\
    '' '$INSTDIR\\\\bin\\\\Coollab-Launcher.exe \\\"%1\\\"'
    System::Call \\\\
    'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    DeleteRegKey HKCR '.coollab'
    DeleteRegKey HKCR 'CoollabFile'
")

# MacOS Bundle specifics
set(CPACK_BUNDLE_NAME "Coollab Launcher")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/Lab/app-resources/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/Lab/app-resources/icon.icns")

#
include(CPack)

# ---------------------
# ---Setup the tests---
# ---------------------
list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_executable(Tests-Coollab-Launcher tests/tests.cpp ${SOURCES})
target_compile_definitions(Tests-Coollab-Launcher PRIVATE COOLLAB_LAUNCHER_TESTS)
target_link_libraries(Tests-Coollab-Launcher PRIVATE Coollab-Launcher-Properties)
target_link_libraries(Tests-Coollab-Launcher PRIVATE doctest::doctest)
set_target_properties(Tests-Coollab-Launcher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/tests/${CMAKE_BUILD_TYPE})
cool_setup(Tests-Coollab-Launcher)
