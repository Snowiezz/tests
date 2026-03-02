cmake_minimum_required(VERSION 3.14)
project(RocketLeagueChampBot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fix for MSVC cmath/cstdlib missing float functions
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES)
    add_compile_options(/W4 /WX /permissive-)
    # Use latest Windows SDK math headers correctly
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

include_directories(include)

# -------------------------------------------------------
# Demo executable
# -------------------------------------------------------
add_executable(champ_bot_demo src/main.cpp)

# -------------------------------------------------------
# Test executable
# -------------------------------------------------------
add_executable(champ_bot_tests src/tests.cpp)

# CTest integration
enable_testing()
add_test(NAME ChampBotTests COMMAND champ_bot_tests)