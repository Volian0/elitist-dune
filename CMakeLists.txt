cmake_minimum_required(VERSION 3.22.1)

project(elitistdune)

include(FetchContent)

FetchContent_Declare (
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main
)

FetchContent_Declare (
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        main
)

FetchContent_Declare (
    SDL3_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG        main
)

FetchContent_Declare (
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        master
)

FetchContent_Declare (
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        develop
)

FetchContent_Declare (
    freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(SDL3 SDL3_image SDL3_mixer glm nlohmann_json freetype) 

file(GLOB_RECURSE SRC_FILES "src/*.cpp")

file(COPY res/ DESTINATION res/)

if(ANDROID)

    add_library(main SHARED ${SRC_FILES})
    target_link_libraries (
        main
        SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer glm::glm nlohmann_json::nlohmann_json freetype GLESv3
    )
    target_include_directories(main PRIVATE "extern/include")
    set_property(TARGET main PROPERTY CXX_STANDARD 20)

else()

    include(FindOpenGL)
    add_executable(${PROJECT_NAME} ${SRC_FILES})
    target_link_libraries (
        ${PROJECT_NAME}
        SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer glm::glm nlohmann_json::nlohmann_json freetype OpenGL::GLES3
    )
    target_include_directories(${PROJECT_NAME} PRIVATE "extern/include")
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

endif()