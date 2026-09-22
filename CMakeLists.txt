cmake_minimum_required(VERSION 3.21...3.31)

project(Vagyojaka VERSION 5.5.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/icon.rc")
endif()

# Normalise any prefix paths to forward slashes before they reach find_package.
# A native Windows path such as C:\Qt\6.8.1\msvc2022_64 is re-parsed as a string by
# the vcpkg toolchain wrapper, where "\Q" is an invalid escape and configuring fails.
if(DEFINED ENV{QTDIR} AND NOT "$ENV{QTDIR}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{QTDIR}" VAGYOJAKA_QTDIR)
    list(APPEND CMAKE_PREFIX_PATH "${VAGYOJAKA_QTDIR}")
endif()

if(CMAKE_PREFIX_PATH)
    file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH)
endif()

find_package(
    Qt6 6.5
    REQUIRED COMPONENTS
    Core
    Concurrent
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Network
    PrintSupport
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------------------
# Sources
#
# Listed explicitly rather than globbed. A glob only re-runs when CMake happens to
# reconfigure, so a newly added file silently fails to build until someone works out
# why. Explicit lists also make the target's contents reviewable in a diff.
# ---------------------------------------------------------------------------

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    main.cpp

    about.cpp
    about.h
    about.ui
    audiowaveform.cpp
    audiowaveform.h
    audiowaveform.ui
    tool.cpp
    tool.h
    tool.ui
    transcriptgenerator.cpp
    transcriptgenerator.h

    audioplayer/audioplayerwidget.cpp
    audioplayer/audioplayerwidget.h

    config/settingsmanager.cpp
    config/settingsmanager.h

    editor/blockandword.h
    editor/dictionary.cpp
    editor/dictionary.h
    editor/editor.cpp
    editor/editor.h
    editor/highlighter.cpp
    editor/highlighter.h
    editor/transcriptanalysis.cpp
    editor/transcriptanalysis.h
    editor/transcriptedits.cpp
    editor/transcriptedits.h
    editor/transcriptexporter.cpp
    editor/transcriptexporter.h
    editor/transcriptserializer.cpp
    editor/transcriptserializer.h
    editor/texteditor.cpp
    editor/texteditor.h
    editor/wordeditor.cpp
    editor/wordeditor.h

    editor/utilities/changespeakerdialog.h
    editor/utilities/changespeakerdialog.ui
    editor/utilities/equationeditor.cpp
    editor/utilities/equationeditor.h
    editor/utilities/equationeditor.ui
    editor/utilities/findreplacedialog.cpp
    editor/utilities/findreplacedialog.h
    editor/utilities/findreplacedialog.ui
    editor/utilities/keyboardshortcutguide.cpp
    editor/utilities/keyboardshortcutguide.h
    editor/utilities/taglistdisplaywidget.cpp
    editor/utilities/taglistdisplaywidget.h
    editor/utilities/tagselectiondialog.cpp
    editor/utilities/tagselectiondialog.h
    editor/utilities/tagselectiondialog.ui
    editor/utilities/timepropagationdialog.h
    editor/utilities/timepropagationdialog.ui

    git/addurldialog.cpp
    git/addurldialog.h
    git/commitdialog.cpp
    git/commitdialog.h
    git/credentialsdialog.cpp
    git/credentialsdialog.h
    git/git.cpp
    git/git.h
    git/git.ui
    git/git_util.h
    git/remotenamedialog.cpp
    git/remotenamedialog.h

    mediaplayer/fftw3.h
    mediaplayer/mediaplayer.cpp
    mediaplayer/mediaplayer.h
    mediaplayer/playercontrols.cpp
    mediaplayer/playercontrols.h
    mediaplayer/qcustomplot.cpp
    mediaplayer/qcustomplot.h
    mediaplayer/videowidget.cpp
    mediaplayer/videowidget.h
    mediaplayer/utilities/mediasplitter.cpp
    mediaplayer/utilities/mediasplitter.h

    tts/customdelegates.cpp
    tts/customdelegates.h
    tts/lazyloadingmodel.cpp
    tts/lazyloadingmodel.h
    tts/ttsannotator.cpp
    tts/ttsannotator.h
    tts/ttsannotator.ui
    tts/ttsrow.h

    util/logger.cpp
    util/logger.h
    util/scriptrunner.cpp
    util/scriptrunner.h

    convert.qrc
    icons.qrc
    icon.rc
    AppIcon.ico

    ${APP_ICON_RESOURCE_WINDOWS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Qt6::Core
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::MultimediaWidgets
        Qt6::Network
        Qt6::PrintSupport
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "${PROJECT_NAME}"
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "vagyojaka"
    )
endif()

if(WIN32 AND MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wa,-mbig-obj)
endif()

# ---------------------------------------------------------------------------
# Dictionaries
#
# The word lists used to be compiled into the executable through wordlists.qrc,
# 71 MB of them, and read into a QStringList at run time. They are now sorted and
# deduplicated at build time into .dict files that the application memory-maps and
# binary searches, so nothing is loaded or sorted when a language is selected.
# ---------------------------------------------------------------------------

add_executable(dictbuild tools/dictbuild/main.cpp)
set_target_properties(dictbuild PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

set(VAGYOJAKA_LANGUAGES english hindi gujarati marathi sanskrit tamil telugu)
set(VAGYOJAKA_DICT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wordlists")
set(VAGYOJAKA_DICT_FILES "")

foreach(language IN LISTS VAGYOJAKA_LANGUAGES)
    set(_source "${CMAKE_CURRENT_SOURCE_DIR}/editor/wordlists/${language}.txt")
    set(_dict "${VAGYOJAKA_DICT_DIR}/${language}.dict")

    if(NOT EXISTS "${_source}")
        message(WARNING "Word list missing, skipping: ${_source}")
        continue()
    endif()

    add_custom_command(
        OUTPUT "${_dict}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${VAGYOJAKA_DICT_DIR}"
        COMMAND dictbuild "${_source}" "${_dict}"
        DEPENDS dictbuild "${_source}"
        COMMENT "Compiling dictionary: ${language}"
        VERBATIM)

    list(APPEND VAGYOJAKA_DICT_FILES "${_dict}")
endforeach()

add_custom_target(vagyojaka_dictionaries ALL DEPENDS ${VAGYOJAKA_DICT_FILES})
add_dependencies(${PROJECT_NAME} vagyojaka_dictionaries)

if(APPLE)
    # Inside a bundle the executable sits in Contents/MacOS, so the dictionaries have
    # to travel in Contents/Resources rather than next to it.
    set_source_files_properties(${VAGYOJAKA_DICT_FILES} PROPERTIES
        GENERATED TRUE
        MACOSX_PACKAGE_LOCATION Resources/wordlists)
    target_sources(${PROJECT_NAME} PRIVATE ${VAGYOJAKA_DICT_FILES})
endif()

# ---------------------------------------------------------------------------
# Tests
#
# Covers the on-disk binary search, which is the part of the dictionary change that
# would fail silently: a wrong comparison just makes correct words look misspelled.
# Run with: ctest --preset <preset>, or ctest --test-dir build/<preset>
# ---------------------------------------------------------------------------

option(VAGYOJAKA_BUILD_TESTS "Build the test suite" ON)

if(VAGYOJAKA_BUILD_TESTS)
    enable_testing()

    add_executable(dictionarytest tests/dictionarytest.cpp editor/dictionary.cpp)
    set_target_properties(dictionarytest PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF)
    target_link_libraries(dictionarytest PRIVATE Qt6::Core)
    add_dependencies(dictionarytest vagyojaka_dictionaries)

    # The test resolves dictionaries relative to its own location, so it has to sit
    # beside the generated wordlists directory.
    set_target_properties(dictionarytest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    add_test(NAME dictionary COMMAND dictionarytest)

    add_executable(serializertest tests/serializertest.cpp editor/transcriptserializer.cpp)
    set_target_properties(serializertest PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    target_link_libraries(serializertest PRIVATE Qt6::Core)
    add_test(NAME serializer COMMAND serializertest)

    add_executable(editstest tests/editstest.cpp editor/transcriptedits.cpp)
    set_target_properties(editstest PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    target_link_libraries(editstest PRIVATE Qt6::Core)
    add_test(NAME edits COMMAND editstest)

    add_executable(analysistest tests/analysistest.cpp
        editor/transcriptanalysis.cpp
        editor/dictionary.cpp)
    set_target_properties(analysistest PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    target_link_libraries(analysistest PRIVATE Qt6::Core)
    add_test(NAME analysis COMMAND analysistest)

    set_tests_properties(dictionary serializer edits analysis PROPERTIES
        ENVIRONMENT "QT_ASSUME_STDERR_HAS_CONSOLE=1")
endif()

# ---------------------------------------------------------------------------
# Dependencies
#
# Every dependency is resolved the same way on every platform:
#   1. a CMake package config (what vcpkg, Conan and most distros ship), then
#   2. pkg-config, then
#   3. a user supplied root directory, then
#   4. a fatal error that says exactly what to do about it.
#
# No absolute paths are baked in. Point CMAKE_TOOLCHAIN_FILE at vcpkg, or set
# FFMPEG_ROOT, and configuring works on any machine.
# ---------------------------------------------------------------------------

find_package(PkgConfig QUIET)

# --- libgit2 ---------------------------------------------------------------

set(VAGYOJAKA_LIBGIT2_TARGET "")

find_package(libgit2 CONFIG QUIET)
if(TARGET libgit2::libgit2package)
    set(VAGYOJAKA_LIBGIT2_TARGET libgit2::libgit2package)
elseif(TARGET libgit2::libgit2)
    set(VAGYOJAKA_LIBGIT2_TARGET libgit2::libgit2)
endif()

if(NOT VAGYOJAKA_LIBGIT2_TARGET)
    find_package(unofficial-libgit2 CONFIG QUIET)
    if(TARGET unofficial::libgit2::libgit2)
        set(VAGYOJAKA_LIBGIT2_TARGET unofficial::libgit2::libgit2)
    endif()
endif()

if(NOT VAGYOJAKA_LIBGIT2_TARGET AND PkgConfig_FOUND)
    pkg_check_modules(LIBGIT2 QUIET IMPORTED_TARGET libgit2)
    if(TARGET PkgConfig::LIBGIT2)
        set(VAGYOJAKA_LIBGIT2_TARGET PkgConfig::LIBGIT2)
    endif()
endif()

if(NOT VAGYOJAKA_LIBGIT2_TARGET)
    message(FATAL_ERROR
        "libgit2 was not found.\n"
        "  Windows: configure with the vcpkg toolchain (see CMakePresets.json), "
        "or run 'vcpkg install libgit2'.\n"
        "  Debian/Ubuntu: sudo apt install libgit2-dev\n"
        "  macOS: brew install libgit2")
endif()

message(STATUS "libgit2: ${VAGYOJAKA_LIBGIT2_TARGET}")
target_link_libraries(${PROJECT_NAME} PRIVATE ${VAGYOJAKA_LIBGIT2_TARGET})

# --- FFTW3 -----------------------------------------------------------------

set(VAGYOJAKA_FFTW3_TARGET "")

find_package(FFTW3 CONFIG QUIET)
if(TARGET FFTW3::fftw3)
    set(VAGYOJAKA_FFTW3_TARGET FFTW3::fftw3)
endif()

if(NOT VAGYOJAKA_FFTW3_TARGET AND PkgConfig_FOUND)
    pkg_check_modules(FFTW3 QUIET IMPORTED_TARGET fftw3)
    if(TARGET PkgConfig::FFTW3)
        set(VAGYOJAKA_FFTW3_TARGET PkgConfig::FFTW3)
    endif()
endif()

if(NOT VAGYOJAKA_FFTW3_TARGET)
    message(FATAL_ERROR
        "FFTW3 was not found.\n"
        "  Windows: configure with the vcpkg toolchain (see CMakePresets.json), "
        "or run 'vcpkg install fftw3'.\n"
        "  Debian/Ubuntu: sudo apt install libfftw3-dev\n"
        "  macOS: brew install fftw")
endif()

message(STATUS "FFTW3: ${VAGYOJAKA_FFTW3_TARGET}")
target_link_libraries(${PROJECT_NAME} PRIVATE ${VAGYOJAKA_FFTW3_TARGET})

# --- FFmpeg ----------------------------------------------------------------

set(FFMPEG_ROOT "$ENV{FFMPEG_ROOT}" CACHE PATH
    "Root of a prebuilt FFmpeg (the directory holding include/ and lib/). Only needed when pkg-config cannot find FFmpeg.")

set(VAGYOJAKA_FFMPEG_RESOLVED FALSE)

if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG QUIET IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libavdevice)
    if(TARGET PkgConfig::FFMPEG)
        message(STATUS "FFmpeg: pkg-config")
        target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
        set(VAGYOJAKA_FFMPEG_RESOLVED TRUE)
    endif()
endif()

if(NOT VAGYOJAKA_FFMPEG_RESOLVED AND FFMPEG_ROOT)
    set(VAGYOJAKA_FFMPEG_OK TRUE)
    foreach(component avutil avcodec avformat avdevice)
        find_path(FFMPEG_INCLUDE_DIR_${component}
            NAMES "lib${component}/${component}.h"
            HINTS "${FFMPEG_ROOT}/include"
            NO_DEFAULT_PATH)
        find_library(FFMPEG_LIBRARY_${component}
            NAMES ${component}
            HINTS "${FFMPEG_ROOT}/lib"
            NO_DEFAULT_PATH)

        if(NOT FFMPEG_INCLUDE_DIR_${component} OR NOT FFMPEG_LIBRARY_${component})
            set(VAGYOJAKA_FFMPEG_OK FALSE)
        else()
            target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIR_${component}})
            target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARY_${component}})
        endif()
    endforeach()

    if(VAGYOJAKA_FFMPEG_OK)
        message(STATUS "FFmpeg: ${FFMPEG_ROOT}")
        set(VAGYOJAKA_FFMPEG_RESOLVED TRUE)
    endif()
endif()

if(NOT VAGYOJAKA_FFMPEG_RESOLVED)
    message(FATAL_ERROR
        "FFmpeg (avcodec, avformat, avutil, avdevice) was not found.\n"
        "  Windows: download a shared build from https://github.com/BtbN/FFmpeg-Builds/releases "
        "and configure with -DFFMPEG_ROOT=<extracted directory>, or set the FFMPEG_ROOT "
        "environment variable. Alternatively install the vcpkg 'ffmpeg' feature of this project.\n"
        "  Debian/Ubuntu: sudo apt install libavcodec-dev libavformat-dev libavutil-dev libavdevice-dev\n"
        "  macOS: brew install ffmpeg")
endif()
