cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(log-lib log)
find_library(android-lib android)

# 添加 native_app_glue
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue
)

add_library(
    novaflarehelper
    SHARED
    main.cpp
)

target_include_directories(novaflarehelper PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(
    novaflarehelper
    android
    native_app_glue
    ${log-lib}
    EGL
    GLESv1_CM
)
