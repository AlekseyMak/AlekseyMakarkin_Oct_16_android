# For more information about using CMake with Android Studio, play the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

# configure import libs

include(ndk-stl-config.cmake)

set(Boost_USE_MULTITHREADED ON)

set(boost_root ${CMAKE_SOURCE_DIR}/src/main/cpp/alanbase/boost)
set(boost_include ${boost_root})

set(ffmpeg_root ${CMAKE_SOURCE_DIR}/src/main/cpp/ffmpeg)
set(ffmpeg_include ${ffmpeg_root}/include/${ANDROID_ABI})
link_directories(${ffmpeg_root}/lib/${ANDROID_ABI})

set(openssl_root ${CMAKE_SOURCE_DIR}/src/main/cpp/openssl)
set(openssl_include ${openssl_root}/include)
link_directories(${openssl_root}/lib/${ANDROID_ABI})

set(alanbase_root ${CMAKE_SOURCE_DIR}/src/main/cpp/alanbase)

include_directories( ${alanbase_root}/src ${alanbase_root}/lib ${boost_include} ${openssl_include} ${ffmpeg_include})

file(GLOB_RECURSE src_AlanBase src/main/cpp/alanbase/*.cpp)

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${src_AlanBase}
        src/main/cpp/AudioManager.cpp
        src/main/cpp/native.cpp)

target_include_directories(native-lib PUBLIC ${alanbase_root}/src ${alanbase_root}/lib ${boost_include} ${openssl_include} ${ffmpeg_include})

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

find_library( # Sets the name of the path variable.
        android-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        android)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib

        c++_shared
        ssl
        crypto
        boost_filesystem
        boost_system
        boost_program_options
#        ffmpeg
        swresample
        avcodec
        avformat
        avutil
        ${android-lib}
        ${log-lib})