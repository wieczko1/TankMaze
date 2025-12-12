cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(main
    src/main.cpp
    src/GameEngine.cpp
    src/AssetManager.cpp
    src/Scene.cpp
    src/TestScene.cpp
    src/Entity.cpp
    src/EntityManager.cpp

    include/GameEngine.h
    include/AssetManager.h
    include/Scene.h
    include/TestScene.h
    include/Components.h
    include/Entity.h
    include/EntityManager.h
)

add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:main>/assets
)

target_include_directories(main
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(main PRIVATE cxx_std_17)
target_link_libraries(main PRIVATE SFML::Graphics)
