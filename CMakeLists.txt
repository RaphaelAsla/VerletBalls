cmake_minimum_required(VERSION 3.16)
set(PROJECT_NAME VerletBalls)
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Werror -O3")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_BUILD_TYPE Debug)

find_package(SFML 2.5 COMPONENTS graphics window system)

file(GLOB_RECURSE source_files 
	"src/*.cpp"
	"src/renderer/*.hpp"
	"src/engine/common/*.hpp"
	"src/engine/physics/*.hpp"
)

set(SOURCES ${source_files})

add_executable(VerletBalls ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "src")
set(SFML_LIBS sfml-system sfml-window sfml-graphics sfml-audio)
target_link_libraries(${PROJECT_NAME} ${SFML_LIBS})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
