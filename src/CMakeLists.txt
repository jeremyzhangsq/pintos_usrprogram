cmake_minimum_required(VERSION 3.8)
project(src)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

include_directories("lib"
                    "lib/kernel"
                    "lib/user"
                    "userprog")

file(GLOB SOURCES "lib/*.c"
        "filesys/*.c"
        "threads/*.c"
        "userprog/*.c"
        "utils/*.c"
        "tests/*.c"
        "examples/*.c"
        "devices/*.c"
        "lib/*.h"
        "filesys/*.h"
        "threads/*.h"
        "userprog/*.h"
        "utils/*.h"
        "tests/*.h"
        "examples/*.h"
        "devices/*.h"
        )

add_executable(src ${SOURCES})



