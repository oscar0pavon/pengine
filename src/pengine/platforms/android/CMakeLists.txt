cmake_minimum_required(VERSION 3.7.0)
project(PavonEngineAndroid)
set(CMAKE_SYSTEM_NAME Android)
set(ANDROID_NDK ~/android-ndk-r25c)
set(CMAKE_ANDROID_NDK ${ANDROID_NDK})
set(CMAKE_ANDROID_ARCH_ABI armabi-v7a)


set(CMAKE_C_STANDARD_REQUIRED ON)

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

include(FindFreetype)
include_directories(${FREETYPE_INCLUDE_DIRS})

include_directories(${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ~/PavonEngine/lib)

add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)


# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")



add_compile_definitions(PUBLIC ANDROID)
add_compile_definitions(PUBLIC DEBUG)


include_directories(~/PavonEngine/source/)

include_directories(
    ${ANDROID_NDK}/sources/android/native_app_glue)

include(~/PavonEngine/include.cmake)


set(ANDROID_ABI arm64-v8a)
#set(ANDROID_ABI armeabi-v7a)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib/${ANDROID_ABI})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib/${ANDROID_ABI})

add_library(pavon_engine_android SHARED main.c)
    

target_include_directories(pavon_engine_android PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_directories(pavon_engine_android PUBLIC ~/PavonEngine/lib)
target_link_directories(pavon_engine_android PUBLIC ${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/33/)
target_link_libraries(pavon_engine_android
    ${android_link})

#set(CMAKE_ANDROID_ARCH_ABI ${CMAKE_ANDROID_ARCH_ABI})
message("My Android Arch ABI ${CMAKE_ANDROID_ARCH_ABI}")
