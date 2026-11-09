# OpenRCT2 UI (main binary)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif ()

# Third party libraries
if (EMSCRIPTEN)
    set(USE_FLAGS "${EMSCRIPTEN_FLAGS}")
    if (NOT DISABLE_VORBIS)
        set(USE_FLAGS "${USE_FLAGS} -s USE_VORBIS=1 -s USE_OGG=1")
    endif ()
    set(SHARED_FLAGS "-fexceptions")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS} ${SHARED_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_LDFLAGS} --bind ${SHARED_FLAGS} -sEXPORTED_FUNCTIONS=${EMSCRIPTEN_EXPORTED_FUNCTIONS} --js-library ${ROOT_DIR}/emscripten/deps.js")
elseif (OPENRCT2_IOS)
    find_package(SDL2 CONFIG REQUIRED)
elseif (MSVC)
    find_package(SDL2 REQUIRED)
else ()
    PKG_CHECK_MODULES(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    if (NOT DISABLE_FLAC)
        PKG_CHECK_MODULES(FLAC REQUIRED IMPORTED_TARGET flac)
    endif ()
    if (NOT DISABLE_VORBIS)
        PKG_CHECK_MODULES(OGG REQUIRED IMPORTED_TARGET ogg)
        PKG_CHECK_MODULES(VORBISFILE REQUIRED IMPORTED_TARGET vorbisfile)
    endif ()
endif ()

if (NOT DISABLE_OPENGL AND NOT EMSCRIPTEN)
    # GL doesn't work nicely with macOS, while find_package doesn't work with multiarch on Ubuntu.
    if (APPLE OR HAIKU)
        find_package(OpenGL REQUIRED)
    elseif (NOT WIN32)
        PKG_CHECK_MODULES(GL REQUIRED gl)
    endif ()
endif ()

# Sources
file(GLOB_RECURSE OPENRCT2_UI_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE OPENRCT2_UI_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

if (APPLE)
    file(GLOB_RECURSE OPENRCT2_UI_MM_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.mm")
    if (OPENRCT2_IOS)
        list(APPEND OPENRCT2_UI_MM_SOURCES "${ROOT_DIR}/ios/platform/UiContext.iOS.mm")
    endif ()
    set_source_files_properties(${OPENRCT2_UI_MM_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++ -fmodules")
endif ()

# Outputs
# Desktop builds own their process. On iOS, SDL's UIKit bootstrap owns the
# process and calls SDL_main, so expose the same UI sources as a static library
# that the app target can link.
if (OPENRCT2_IOS)
    set(OPENRCT2_UI_TARGET openrct2-ui)
    add_library(${OPENRCT2_UI_TARGET} STATIC ${OPENRCT2_UI_SOURCES} ${OPENRCT2_UI_MM_SOURCES})
    add_library(OpenRCT2::openrct2-ui ALIAS ${OPENRCT2_UI_TARGET})
else ()
    set(OPENRCT2_UI_TARGET openrct2)
    add_executable(${OPENRCT2_UI_TARGET} ${OPENRCT2_UI_SOURCES} ${OPENRCT2_UI_MM_SOURCES})
    add_executable(OpenRCT2::openrct2 ALIAS ${OPENRCT2_UI_TARGET})
endif ()

SET_CHECK_CXX_FLAGS(${OPENRCT2_UI_TARGET})
ipo_set_target_properties(${OPENRCT2_UI_TARGET})

# mingw builds cannot use the PkgConfig imported targets
if (EMSCRIPTEN)
    target_link_libraries(${OPENRCT2_UI_TARGET} "libopenrct2"
                                    ${ICU_DATA_LIBRARIES}
                                    ${ICU_DT_LIBRARY_RELEASE})
elseif (OPENRCT2_IOS)
    if (TARGET SDL2::SDL2-static)
        set(OPENRCT2_SDL_TARGET SDL2::SDL2-static)
    elseif (TARGET SDL2::SDL2)
        set(OPENRCT2_SDL_TARGET SDL2::SDL2)
    else ()
        message(FATAL_ERROR "SDL2 did not provide a linkable CMake target")
    endif ()
    target_link_libraries(${OPENRCT2_UI_TARGET} "libopenrct2" ${OPENRCT2_SDL_TARGET})
elseif (NOT MSVC AND NOT WIN32)
    target_link_libraries(${OPENRCT2_UI_TARGET} "libopenrct2"
                                    PkgConfig::SDL2)
else ()
    target_link_libraries(${OPENRCT2_UI_TARGET} "libopenrct2"
                                    ${SDL2_LDFLAGS})
endif ()
target_link_platform_libraries(${OPENRCT2_UI_TARGET})

if (NOT DISABLE_FLAC)
    if (NOT MSVC AND NOT WIN32)
        target_link_libraries(${OPENRCT2_UI_TARGET} PkgConfig::FLAC)
    else ()
        target_link_libraries(${OPENRCT2_UI_TARGET} ${FLAC_LDFLAGS})
    endif ()
endif ()

if (NOT DISABLE_VORBIS)
    if (NOT MSVC AND NOT WIN32 AND NOT EMSCRIPTEN)
        target_link_libraries(${OPENRCT2_UI_TARGET} PkgConfig::OGG PkgConfig::VORBISFILE)
    else ()
        target_link_libraries(${OPENRCT2_UI_TARGET} ${OGG_LDFLAGS} ${VORBISFILE_LDFLAGS})
    endif ()
endif ()

if (NOT DISABLE_OPENGL)
    if (WIN32)
        target_link_libraries(${OPENRCT2_UI_TARGET} opengl32)
    elseif (APPLE OR HAIKU)
        target_link_libraries(${OPENRCT2_UI_TARGET} ${OPENGL_LIBRARY})
    else ()
        target_link_libraries(${OPENRCT2_UI_TARGET} ${GL_LIBRARIES})
    endif ()
endif ()

# Includes
target_include_directories(${OPENRCT2_UI_TARGET} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/.."
                                              ${OGG_INCLUDE_DIRS}
                                              ${VORBIS_INCLUDE_DIRS}
                                              ${FLAC_INCLUDE_DIRS})
target_include_directories(${OPENRCT2_UI_TARGET} SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS}
                                                     "${CMAKE_CURRENT_LIST_DIR}/../thirdparty")

# Compiler flags
if (WIN32)
    # mingw complains about "%zu" not being a valid format specifier for printf, unless we
    # tell it that it is
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USE_MINGW_ANSI_STDIO=1")
    target_link_libraries(${OPENRCT2_UI_TARGET} comdlg32)
    if (MSVC)
        target_link_libraries(${OPENRCT2_UI_TARGET} SDL2::SDL2-static)
        target_include_directories(${OPENRCT2_UI_TARGET} SYSTEM PRIVATE SDL2::SDL2-static)
    else ()
        # mingw does not provide proper CMake information like other configurations
        find_path(SDL2_INCLUDE_DIRS SDL2/SDL.h)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            find_library(SDL2_LDFLAGS sdl2d)
        else ()
            find_library(SDL2_LDFLAGS sdl2)
        endif ()
        # Hardcode some of the libraries used by mingw builds
        target_link_libraries(${OPENRCT2_UI_TARGET} imm32 winmm setupapi version)
    endif ()
endif ()
if (MSVC)
    # Add DPI-aware manifest
    # HACK Add /MANIFEST:NO first to prevent cmake from adding its own manifest arguments
    if (NOT $ENV{VCToolsInstallDir} STREQUAL "")
        set(MANIFEST_DPI "$ENV{VCToolsInstallDir}\\Include\\Manifest\\PerMonitorHighDPIAware.manifest")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO /MANIFEST /manifest:embed /manifestinput:\"${MANIFEST_DPI}\"")
    endif ()
endif ()

# Defines
target_compile_definitions(${OPENRCT2_UI_TARGET} PUBLIC
    $<$<BOOL:${DISABLE_FLAC}>:DISABLE_FLAC>
    $<$<BOOL:${DISABLE_VORBIS}>:DISABLE_VORBIS>
    # If OpenGL is enabled then we want to dynamically look up symbols.
    # If its disabled and somehow we magically end up linking to OpenGL we don't want look up the OpenGL symbols????
    $<IF:$<BOOL:${DISABLE_OPENGL}>,DISABLE_OPENGL,OPENGL_NO_LINK>
)

if (MSVC)
    add_definitions(-D__DISABLE_DLL_PROXY__)
endif ()

# Add headers check to verify all headers carry their dependencies.
# Only valid for Clang for now:
# - GCC 8 does not support -Wno-pragma-once-outside-header
# - Other compilers status unknown
if (ENABLE_HEADERS_CHECK AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(OPENRCT2_HEADERS_CHECK ${OPENRCT2_UI_HEADERS})
    # OpenGLAPIProc.h is not meant to be included directly.
    list(REMOVE_ITEM OPENRCT2_HEADERS_CHECK "${CMAKE_CURRENT_LIST_DIR}/drawing/engines/opengl/OpenGLAPIProc.h")
    add_library(openrct2-headers-check OBJECT ${OPENRCT2_HEADERS_CHECK})
    set_target_properties(openrct2-headers-check PROPERTIES LINKER_LANGUAGE CXX)
    set_source_files_properties(${OPENRCT2_HEADERS_CHECK} PROPERTIES LANGUAGE CXX)
    target_compile_options(openrct2-headers-check PRIVATE -Wno-pragma-once-outside-header -Wno-unused-const-variable)
    get_target_property(OPENRCT2_INCLUDE_DIRS ${OPENRCT2_UI_TARGET} INCLUDE_DIRECTORIES)
    set_target_properties(openrct2-headers-check PROPERTIES INCLUDE_DIRECTORIES "${OPENRCT2_INCLUDE_DIRS}")
endif ()


if(MACOS_BUNDLE)
    add_dependencies(${OPENRCT2_UI_TARGET} openrct2-cli)
    set(OUTPUT_NAME "OpenRCT2")
    set(MACOS_APP_NAME "${OUTPUT_NAME}.app")
    set(BUNDLE_FRAMEWORK_DIR "${MACOS_APP_NAME}/Contents/Frameworks")
    set(BUNDLE_RESOURCE_DIR "${MACOS_APP_NAME}/Contents/Resources")
    set(SOURCE_DATA_DIR "${ROOT_DIR}/data")

    # Add distribution sources
    target_sources(${OPENRCT2_UI_TARGET}
        PUBLIC ${ROOT_DIR}/distribution/readme.txt
        PUBLIC ${ROOT_DIR}/distribution/changelog.txt
        PUBLIC ${ROOT_DIR}/contributors.md
        PUBLIC ${ROOT_DIR}/PRIVACY.md
        PUBLIC ${ROOT_DIR}/resources/mac/openrct2.icns
        PUBLIC ${ROOT_DIR}/resources/mac/Assets.car
        )

    # Specify the resources to move to the bundle
    set(BUNDLE_RESOURCES
        ${ROOT_DIR}/distribution/readme.txt
        ${ROOT_DIR}/distribution/changelog.txt
        ${ROOT_DIR}/contributors.md
        ${ROOT_DIR}/PRIVACY.md
        ${ROOT_DIR}/resources/mac/openrct2.icns
        ${ROOT_DIR}/resources/mac/Assets.car
        )

    if (CMAKE_OSX_ARCHITECTURES MATCHES "${SYSTEM_MACOS_ARCH}")
        target_sources(${OPENRCT2_UI_TARGET} PUBLIC ${CMAKE_BINARY_DIR}/g2.dat)
        list(APPEND BUNDLE_RESOURCES ${CMAKE_BINARY_DIR}/g2.dat)
        target_sources(${OPENRCT2_UI_TARGET} PUBLIC ${CMAKE_BINARY_DIR}/fonts.dat)
        list(APPEND BUNDLE_RESOURCES ${CMAKE_BINARY_DIR}/fonts.dat)
        target_sources(${OPENRCT2_UI_TARGET} PUBLIC ${CMAKE_BINARY_DIR}/tracks.dat)
        list(APPEND BUNDLE_RESOURCES ${CMAKE_BINARY_DIR}/tracks.dat)
        target_sources(${OPENRCT2_UI_TARGET} PUBLIC ${CMAKE_BINARY_DIR}/palettes.dat)
        list(APPEND BUNDLE_RESOURCES ${CMAKE_BINARY_DIR}/palettes.dat)
    endif()


    if(OPENRCT2_BRANCH EQUAL master)
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENRCT2_VERSION_TAG}")
    else()
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENRCT2_VERSION_TAG} ${OPENRCT2_BRANCH}")
    endif()

    set(PRODUCT_BUNDLE_IDENTIFIER "io.openrct2.OpenRCT2")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${OPENRCT2_COMMIT_SHA1_SHORT}")
    set(EXECUTABLE_NAME "${OUTPUT_NAME}")
    set(MACOSX_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    set(PRODUCT_NAME "${OUTPUT_NAME}")

    # Copy data
    file(COPY ${SOURCE_DATA_DIR}/language DESTINATION "${BUNDLE_RESOURCE_DIR}")
    file(COPY ${SOURCE_DATA_DIR}/scenario_patches DESTINATION "${BUNDLE_RESOURCE_DIR}")
    file(COPY ${SOURCE_DATA_DIR}/shaders DESTINATION "${BUNDLE_RESOURCE_DIR}")

    # Download objects and sequences
    set(OBJECTS_DIR ${CMAKE_BINARY_DIR}/object)
    set(TITLE_SEQUENCE_DIR ${CMAKE_BINARY_DIR}/sequence)
    download_openrct2_zip(
        DOWNLOAD_DIR ${OBJECTS_DIR}
        ZIP_URL ${OBJECTS_URL}
        SHA256 ${OBJECTS_SHA256}
    )

    download_openrct2_zip(
        DOWNLOAD_DIR ${TITLE_SEQUENCE_DIR}
        ZIP_URL ${TITLE_SEQUENCE_URL}
        SHA256 ${TITLE_SEQUENCE_SHA256}
    )

    # Download opensfx and openmusic
    set(ASSET_PACK_DIR ${CMAKE_BINARY_DIR}/assetpack)
    download_openrct2_zip(
        DOWNLOAD_DIR ${CMAKE_BINARY_DIR}
        ZIP_URL ${OPENSFX_URL}
        SHA256 ${OPENSFX_SHA256}
    )
    download_openrct2_zip(
        DOWNLOAD_DIR ${CMAKE_BINARY_DIR}
        ZIP_URL ${OPENMUSIC_URL}
        SHA256 ${OPENMUSIC_SHA256}
    )

    # Copy downloaded data
    file(COPY ${OBJECTS_DIR} DESTINATION "${BUNDLE_RESOURCE_DIR}")
    file(COPY ${TITLE_SEQUENCE_DIR} DESTINATION "${BUNDLE_RESOURCE_DIR}")
    file(COPY ${ASSET_PACK_DIR} DESTINATION "${BUNDLE_RESOURCE_DIR}")

    # Create as a bundle
    set_target_properties(${OPENRCT2_UI_TARGET} PROPERTIES
        MACOSX_BUNDLE ON
        OUTPUT_NAME ${OUTPUT_NAME}
        MACOSX_BUNDLE_BUNDLE_NAME ${OUTPUT_NAME}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/distribution/macos/Info.plist
        RESOURCE "${BUNDLE_RESOURCES}")


    install(CODE "
        include(BundleUtilities)
        fixup_bundle(${CMAKE_BINARY_DIR}/${MACOS_APP_NAME} \"\" \"\")
        "  BUNDLE DESTINATION ${CMAKE_BINARY_DIR}
    )
endif ()
