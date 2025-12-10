cmake_minimum_required(VERSION 3.15)
project(Velocity)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходники
add_executable(Velocity
    src/main.cpp
)

# Windows библиотеки
target_link_libraries(Velocity PRIVATE
    kernel32
    user32
)

# Настройки для Windows
if(WIN32)
    target_compile_definitions(Velocity PRIVATE
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
endif()
