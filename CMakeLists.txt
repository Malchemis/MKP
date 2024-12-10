cmake_minimum_required(VERSION 3.30)
project(MKP C)

set(CMAKE_C_STANDARD 23)

add_executable(MKP main.c
        data.c
        utils.c
        lib/data.h
        lib/utils.h)

target_include_directories(MKP PUBLIC Instances_MKP)