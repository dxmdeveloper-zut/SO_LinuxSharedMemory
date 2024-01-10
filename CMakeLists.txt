cmake_minimum_required(VERSION 3.20)
project(receiver C)
project(sender C)

set(CMAKE_C_STANDARD 90)
add_compile_options(-pedantic -Wall -Werror)

add_executable(receiver
        receiver.c)

add_executable(sender
        sender.c)
