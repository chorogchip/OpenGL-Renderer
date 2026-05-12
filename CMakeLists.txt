cmake_minimum_required(VERSION 3.20)
project(OpenGL-Renderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(SPONZA_SCENE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/main_sponza")
set(SPONZA_SCENE_FILE "${SPONZA_SCENE_DIR}/NewSponza_Main_glTF_003.gltf")
set(SPONZA_SCENE_URL "https://cdrdv2.intel.com/v1/dl/getContent/830833")
set(SPONZA_SCENE_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/downloads/intel_sponza_base_scene.zip")
option(OPENGL_RENDERER_DOWNLOAD_SPONZA "Download Intel Sponza Base Scene when assets/main_sponza is missing" ON)

if(NOT EXISTS "${SPONZA_SCENE_FILE}")
    if(NOT OPENGL_RENDERER_DOWNLOAD_SPONZA)
        message(FATAL_ERROR
            "Missing ${SPONZA_SCENE_FILE}. Reconfigure with "
            "-DOPENGL_RENDERER_DOWNLOAD_SPONZA=ON to download the Intel Sponza scene.")
    endif()

    message(STATUS "Intel Sponza scene not found; downloading from ${SPONZA_SCENE_URL}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/downloads")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    file(
        DOWNLOAD "${SPONZA_SCENE_URL}" "${SPONZA_SCENE_ARCHIVE}"
        SHOW_PROGRESS
        STATUS SPONZA_DOWNLOAD_STATUS
        TLS_VERIFY ON
    )
    list(GET SPONZA_DOWNLOAD_STATUS 0 SPONZA_DOWNLOAD_RESULT)
    list(GET SPONZA_DOWNLOAD_STATUS 1 SPONZA_DOWNLOAD_MESSAGE)
    if(NOT SPONZA_DOWNLOAD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download Intel Sponza scene: ${SPONZA_DOWNLOAD_MESSAGE}")
    endif()

    message(STATUS "Extracting Intel Sponza scene to ${CMAKE_CURRENT_SOURCE_DIR}/assets")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xzf "${SPONZA_SCENE_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        RESULT_VARIABLE SPONZA_EXTRACT_RESULT
    )
    if(NOT SPONZA_EXTRACT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract Intel Sponza scene archive: ${SPONZA_SCENE_ARCHIVE}")
    endif()

    if(NOT EXISTS "${SPONZA_SCENE_FILE}")
        message(FATAL_ERROR
            "Downloaded Intel Sponza archive did not produce ${SPONZA_SCENE_FILE}. "
            "Check the archive layout from ${SPONZA_SCENE_URL}.")
    endif()
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/CMakeLists.txt")
    add_subdirectory(external/glfw)
else()
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(glfw)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glm/CMakeLists.txt")
    add_subdirectory(external/glm)
else()
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.3
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(glm)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/fastgltf/CMakeLists.txt")
    add_subdirectory(external/fastgltf)
else()
    FetchContent_Declare(
        fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG v0.9.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(fastgltf)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include/glad/glad.h" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c")
    add_library(glad STATIC external/glad/src/glad.c)
    add_library(glad::glad ALIAS glad)
    target_include_directories(glad PUBLIC external/glad/include)
else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v0.1.36
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_GetProperties(glad)
    if(NOT glad_POPULATED)
        FetchContent_Populate(glad)
    endif()
    add_subdirectory(${glad_SOURCE_DIR}/cmake ${glad_BINARY_DIR}/cmake)
    glad_add_library(glad REPRODUCIBLE LOADER API gl:compatibility=4.6)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image.h")
    set(STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
else()
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_GetProperties(stb)
    if(NOT stb_POPULATED)
        FetchContent_Populate(stb)
    endif()
    set(STB_INCLUDE_DIR "${stb_SOURCE_DIR}")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp")
    set(IMGUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
else()
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.7
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()
    set(IMGUI_SOURCE_DIR "${imgui_SOURCE_DIR}")
endif()

find_package(OpenGL REQUIRED)

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_SOURCE_DIR}
    ${IMGUI_SOURCE_DIR}/backends
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui PUBLIC glfw glad::glad OpenGL::GL)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    OPENGL_RENDERER_DEFAULT_SPONZA_SCENE="${SPONZA_SCENE_FILE}"
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${STB_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    glad::glad
    glm::glm
    imgui
    fastgltf::fastgltf
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/texture.png"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/texture.png"
)
