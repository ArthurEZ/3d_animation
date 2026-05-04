cmake_minimum_required(VERSION 3.20)
project(CharacterAnimator LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch external dependencies
FetchContent_Declare(
  glfw
  URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Handle Assimp
if(APPLE)
    set(ASSIMP_INCLUDE_DIR "/opt/homebrew/include")
    set(ASSIMP_LIBRARY "/opt/homebrew/lib/libassimp.dylib")
    include_directories(${ASSIMP_INCLUDE_DIR})
else()
    find_package(ASSIMP QUIET)
    if(ASSIMP_FOUND)
        set(ASSIMP_LIBRARY ${ASSIMP_LIBRARIES})
    else()
        message(STATUS "ASSIMP not found. Fetching from source...")
        # Prefer building Assimp as a static library to avoid runtime ABI issues
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
        set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
        set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
          assimp
          URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.3.zip
          DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        )
        FetchContent_MakeAvailable(assimp)
        set(ASSIMP_LIBRARY assimp)
    endif()
endif()

FetchContent_MakeAvailable(glfw)

# Use the project's bundled GLM headers (avoid fetching during build)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../project/includes/glm)

# GLAD source files - reference from project
set(GLAD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../project/src/glad.c
)

# stb_image
set(STB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../project/includes)

# Create executable
add_executable(CharacterAnimator
    src/main.cpp
    src/math.cpp
    src/graphics.cpp
    src/assets.cpp
    ${GLAD_SOURCES}
)

target_include_directories(CharacterAnimator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../project/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../project/includes/glad
    ${STB_DIR}
)

# Ensure Assimp headers are searched before bundled project headers.
if(TARGET assimp)
    target_include_directories(CharacterAnimator BEFORE PRIVATE
        $<TARGET_PROPERTY:assimp,INTERFACE_INCLUDE_DIRECTORIES>
    )
elseif(ASSIMP_FOUND)
    target_include_directories(CharacterAnimator BEFORE PRIVATE ${ASSIMP_INCLUDE_DIRS})
endif()

target_link_libraries(CharacterAnimator PRIVATE
    glfw
    ${ASSIMP_LIBRARY}
    $<$<PLATFORM_ID:Windows>:opengl32>
    $<$<PLATFORM_ID:Linux>:GL>
    $<$<PLATFORM_ID:Darwin>:-framework OpenGL>
)

# Add GLM include directory explicitly (we use bundled headers)
target_include_directories(CharacterAnimator PRIVATE ${GLM_INCLUDE_DIR})

# Set working directory to build folder for resources
set_target_properties(CharacterAnimator PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Copy shaders to build directory
add_custom_command(TARGET CharacterAnimator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_BINARY_DIR}/shaders
    COMMENT "Copying shaders to build directory"
)

# Copy resources symlink/copy from parent project
add_custom_command(TARGET CharacterAnimator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../project/resources
    ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying resources to build directory"
)

# For MinGW builds on Windows, copy required runtime DLLs beside the executable.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(MINGW_RUNTIME_DLLS
        libstdc++-6.dll
        libwinpthread-1.dll
        libgcc_s_seh-1.dll
        libgcc_s_dw2-1.dll
        libgcc_s_sjlj-1.dll
    )

    foreach(dll_name IN LISTS MINGW_RUNTIME_DLLS)
        set(dll_path "${MINGW_BIN_DIR}/${dll_name}")
        if(EXISTS "${dll_path}")
            add_custom_command(TARGET CharacterAnimator POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll_path}"
                "$<TARGET_FILE_DIR:CharacterAnimator>"
                COMMENT "Copying MinGW runtime: ${dll_name}"
            )
        endif()
    endforeach()
endif()
