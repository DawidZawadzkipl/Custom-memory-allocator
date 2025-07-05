cmake_minimum_required(VERSION 3.26)
project(custom_memory_allocator C)

set(CMAKE_C_STANDARD 11)

add_executable(custom_memory_allocator main.c
        heap.h
        heap.c)
