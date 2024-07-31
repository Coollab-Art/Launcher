cmake_minimum_required(VERSION 3.10)
project(Coollab-Launcher)
add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)

# Set the folder where the executable is created
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# Set warning level
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors -Wconversion -Wsign-conversion -Wimplicit-fallthrough)
endif()

# Maybe enable warnings as errors
set(WARNINGS_AS_ERRORS_FOR_COOLLAB_LAUNCHER OFF CACHE BOOL "ON iff you want to treat warnings as errors") # Might be overriden in the CMake cache

if(WARNINGS_AS_ERRORS_FOR_COOLLAB_LAUNCHER)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

include(FetchContent)

# Make sure we link the static version
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static library" FORCE)
set(BULD_STATIC_LIBS ON CACHE BOOL "" FORCE)

# JSON
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# MINIZ
# Configurer FetchContent pour miniz
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG master
)

# Get and build JSON
FetchContent_MakeAvailable(json)

FetchContent_MakeAvailable(miniz)

# # Grab all the source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json miniz)

set(COOL_USE_OPENGL $<IF:$<PLATFORM_ID:Darwin>,410,430>) # Must be >= 330 for ImGui to work properly # We use 410 when building on Mac because it doesn't support higher versions of OpenGL
set(COOL_USE_INSTALLER ON)
add_subdirectory(Lab/Cool)
target_link_libraries(${PROJECT_NAME} PRIVATE Cool::Core)
cool_setup(${PROJECT_NAME})

add_subdirectory(Lab/lib/cpp-httplib)
target_link_libraries(${PROJECT_NAME} PRIVATE httplib)

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
set(CPACK_PACKAGE_NAME "Coollab")
set(CPACK_PACKAGE_VERSION "${COOLLAB_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Coollab") # Don't put this in a Coollab parent folder. In NSIS this messes up the install and prevents our app to appear in "Apps and Features" to uninstall it for example.
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}${SEP}Lab${SEP}app-resources${SEP}installer_banner.bmp") # Seems like it has to be .bmp, at least for NSIS. NB: if there is any transparency in the image, NSIS won't be able to render it at all. Exporting a fully opaque .bmp from Paint.net works. 150x57 pixels is the ideal size to avoid uggly downscale artifacts.
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}${SEP}Lab${SEP}LICENSE.txt")
set(CPACK_PACKAGE_VENDOR "Coollab")
set(CPACK_PACKAGE_EXECUTABLES "Coollab" "Coollab") # Creates Start Menu shortcuts
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

set(CPACK_PACKAGE_FILE_NAME "Coollab-${OS_NAME}")

# NSIS specifics
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\Coollab.exe") # TODO(Launcher)

# .coollab extension
# --- Windows
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    WriteRegStr HKCR '.coollab' '' 'CoollabFile'
    WriteRegStr HKCR 'CoollabFile' '' 'Coollab File'
    WriteRegStr HKCR 'CoollabFile\\\\shell' '' 'open'
    WriteRegStr HKCR 'CoollabFile\\\\DefaultIcon' \\\\
    '' '$INSTDIR\\\\bin\\\\Coollab.exe,0'
    WriteRegStr HKCR 'CoollabFile\\\\shell\\\\open\\\\command' \\\\
    '' '$INSTDIR\\\\bin\\\\Coollab.exe \\\"%1\\\"'
    WriteRegStr HKCR 'CoollabFile\\\\shell\\\\edit\\\\command' \\\\
    '' '$INSTDIR\\\\bin\\\\Coollab.exe \\\"%1\\\"'
    System::Call \\\\
    'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    DeleteRegKey HKCR '.coollab'
    DeleteRegKey HKCR 'CoollabFile'
")

# MacOS Bundle specifics
set(CPACK_BUNDLE_NAME "Coollab")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/Lab/app-resources/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/Lab/app-resources/icon.icns")

#
include(CPack)