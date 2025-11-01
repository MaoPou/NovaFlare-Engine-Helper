cmake_minimum_required(VERSION 3.18.1)

project("NovaFlareHelper")

find_library(log-lib log)

add_library(
    novaflarehelper
    SHARED
    src/main/cpp/main.cpp
)

target_include_directories(
    novaflarehelper 
    PRIVATE 
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(
    novaflarehelper
    android
    ${log-lib}
    EGL
    GLESv1_CM
)
