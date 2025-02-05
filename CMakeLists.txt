cmake_minimum_required(VERSION 3.16)


project(Vagyojaka VERSION 5.5.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# For windows use vcpkg(place it in C: folder) and use MinGW compiler
# and install the libgit
# using "vcpkg install libgit2 --triplet=x64_mingw_dynamic" command from cmd

if(WIN32)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/icon.rc")
    if(MINGW)
        set(CMAKE_PREFIX_PATH "D:/Qt/6.6.1/mingw_64" CACHE PATH "Initial cache" FORCE)
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};C:/vcpkg/installed/x64-mingw-dynamic/share")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

find_package(
    Qt6
    HINTS "$ENV{QTDIR}"
    REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Network
    PrintSupport
)
# find_package(Qt6PrintSupport REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB MEDIAPLAYER_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/*.ui")
file(GLOB MEDIAPLAYER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/*.cpp")
file(GLOB MEDIAPLAYER_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/*.h")

file(GLOB MEDIAPLAYER_UTILS_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/utilities/*.ui")
file(GLOB MEDIAPLAYER_UTILS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/utilities/*.cpp")
file(GLOB MEDIAPLAYER_UTILS_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/utilities/*.h")

file(GLOB EDITOR_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.ui")
file(GLOB EDITOR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.cpp")
file(GLOB EDITOR_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.h")
file(GLOB EDITOR_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.qrc")

file(GLOB EDITOR_UTILS_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/editor/utilities/*.ui")
file(GLOB EDITOR_UTILS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/editor/utilities/*.cpp")
file(GLOB EDITOR_UTILS_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/editor/utilities/*.h")

file(GLOB GIT_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/git/*.ui")
file(GLOB GIT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/git/*.cpp")
file(GLOB GIT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/git/*.h")

file(GLOB PROJECT_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB PROJECT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB PROJECT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB PROJECT_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

# set(GUI_TYPE)
# if (WIN32)
#     set(GUI_TYPE WIN32)
# else ()
#     set(GUI_TYPE)
# endif ()

add_executable(
        ${PROJECT_NAME}
        ${GUI_TYPE} main.cpp

        ${PROJECT_FORMS}
        ${PROJECT_SOURCE}
        ${PROJECT_HEADER}
        ${PROJECT_RESOURCES}

        ${MEDIAPLAYER_FORMS}
        ${MEDIAPLAYER_SOURCE}
        ${MEDIAPLAYER_HEADER}

        ${MEDIAPLAYER_UTILS_FORMS}
        ${MEDIAPLAYER_UTILS_SOURCE}
        ${MEDIAPLAYER_UTILS_HEADER}

        ${EDITOR_FORMS}
        ${EDITOR_SOURCE}
        ${EDITOR_HEADER}
        ${EDITOR_RESOURCES}

        ${EDITOR_UTILS_FORMS}
        ${EDITOR_UTILS_SOURCE}
        ${EDITOR_UTILS_HEADER}

        ${GIT_FORMS}
        ${GIT_SOURCE}
        ${GIT_HEADER}
        tts/ttsannotator.h tts/ttsannotator.cpp tts/ttsannotator.ui
        tts/ttsrow.h
        audioplayer/audioplayerwidget.h
        audioplayer/audioplayerwidget.cpp
        icons.qrc
        icon.rc

        ${APP_ICON_RESOURCE_WINDOWS}
        tts/lazyloadingmodel.h tts/lazyloadingmodel.cpp
        tts/customdelegates.h tts/customdelegates.cpp
        AppIcon.ico
        config/settingsmanager.h
        config/settingsmanager.cpp


)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Qt6::Core
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
        # MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        # MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    )
else()
    # Linux-specific settings
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "vagyojaka"
    )
endif()

if (WIN32 AND MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wa,-mbig-obj
    )
endif()

if (LINUX OR UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::LIBGIT2)
endif()

message("${LIBGIT2_FOUND}")
# find_package(libgit2)
if (NOT LIBGIT2_FOUND)
    if (WIN32 AND MINGW)
        find_package(libgit2 CONFIG QUIET)
        if(libgit2_FOUND)
            message(STATUS "Found official libgit2 package")
            target_link_libraries(${PROJECT_NAME} PRIVATE libgit2::libgit2package)
        else()
            message(STATUS "Official libgit2 package not found, trying unofficial package")
            # Try the unofficial package for MinGW
            if(WIN32 AND MINGW)
                find_package(unofficial-libgit2 CONFIG REQUIRED)
                target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::libgit2::libgit2)
            elseif(LINUX OR UNIX)
                message(FATAL_ERROR "libgit2 is required")
            endif()
        endif()
    elseif (LINUX OR UNIX)
        message(FATAL_ERROR "libgit2 is required")
    endif()
endif()

# find_package(FFmpeg)


# if(FFMPEG_FOUND)
#     message("FFmpeg found.")
#     message("FFmpeg include directories: ${FFMPEG_INCLUDE_DIRS}")
#     message("FFmpeg libraries: ${FFMPEG_LIBRARIES}")
#     target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
#     target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES})

# elseif (WIN32)
if (WIN32)
    find_package(FFTW3 CONFIG REQUIRED)

    if (FFTW3_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE FFTW3::fftw3)
    else()
        message(FATAL_ERROR "FFTW3 is required")
    endif()

    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(FFMEPG_INCLUDE_DIR C:/ffmpeg_x86/include)
        set(FFMEPG_LIB_DIR C:/ffmpeg_x86/lib)
    else()
        set(FFMEPG_INCLUDE_DIR C:/ffmpeg/include)
        set(FFMEPG_LIB_DIR C:/ffmpeg/lib)
    endif()

    FIND_PATH(FFMPEG_INCLUDE_DIR_AVUTIL NAMES libavutil/avutil.h HINTS ${FFMEPG_INCLUDE_DIR})
    FIND_PATH(FFMPEG_INCLUDE_DIR_AVCODEC NAMES libavcodec/avcodec.h HINTS ${FFMEPG_INCLUDE_DIR})
    FIND_PATH(FFMPEG_INCLUDE_DIR_AVFORMAT NAMES libavformat/avformat.h HINTS ${FFMEPG_INCLUDE_DIR})
    FIND_PATH(FFMPEG_INCLUDE_DIR_AVDEVICE NAMES libavdevice/avdevice.h HINTS ${FFMEPG_INCLUDE_DIR})

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${FFMPEG_INCLUDE_DIR_AVUTIL}
        ${FFMPEG_INCLUDE_DIR_AVCODEC}
        ${FFMPEG_INCLUDE_DIR_AVFORMAT}
        ${FFMPEG_INCLUDE_DIR_AVDEVICE}
    )

    FIND_LIBRARY(FFMPEG_AVUTIL_LIBRARY NAMES avutil HINTS ${FFMEPG_LIB_DIR})
    FIND_LIBRARY(FFMPEG_AVCODEC_LIBRARY NAMES avcodec HINTS ${FFMEPG_LIB_DIR})
    FIND_LIBRARY(FFMPEG_AVFORMAT_LIBRARY NAMES avformat HINTS ${FFMEPG_LIB_DIR})
    FIND_LIBRARY(FFMPEG_AVDEVICE_LIBRARY NAMES avdevice HINTS ${FFMEPG_LIB_DIR})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FFMPEG_AVUTIL_LIBRARY}
        ${FFMPEG_AVCODEC_LIBRARY}
        ${FFMPEG_AVFORMAT_LIBRARY}
        ${FFMPEG_AVDEVICE_LIBRARY}
    )

    message(${FFMPEG_AVUTIL_LIBRARY} "\n"
        ${FFMPEG_AVCODEC_LIBRARY} "\n"
        ${FFMPEG_AVFORMAT_LIBRARY} "\n"
        ${FFMPEG_AVDEVICE_LIBRARY} "\n"
    )

elseif(UNIX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(FFTW3 REQUIRED IMPORTED_TARGET fftw3)
    if(FFTW3_FOUND)
        message(STATUS "FFTW3 found")
        target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW3_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::FFTW3)
    else()
        message(FATAL_ERROR "FFTW3 is required")
    endif()

    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
      libavcodec
      libavformat
      libavutil
      libavdevice
    )
    if(FFMPEG_FOUND)
        message(STATUS "FFMPEG found")
        target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
        target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARY_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
    else()
        message(FATAL_ERROR "FFmpeg is required")
    endif()

endif()
