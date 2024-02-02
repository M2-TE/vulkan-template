cmake_minimum_required(VERSION 3.27)
project(vulkan-renderer LANGUAGES C CXX)

# disallow option() from overwriting set()
set (CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Generic CMake Settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)
set(BUILD_SHARED_LIBS OFF)
# check for IPO/LTO support
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_ENABLED LANGUAGES CXX)
message(STATUS "IPO/LTO enabled: ${LTO_ENABLED}")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${LTO_ENABLED})

# SDL Build settings
set(SDL_SHARED              ON)
set(SDL_STATIC              OFF)
set(SDL_TEST_LIBRARY        OFF)
set(SDL_DISABLE_INSTALL     ON)
set(SDL_DISABLE_UNINSTALL   ON)
# SDL Graphics API support
set(SDL_VULKAN      ON)
set(SDL_DIRECTX     OFF)
set(SDL_OPENGL      OFF)
set(SDL_OPENGLES    OFF)
# SDL Subsystems
set(SDL_AUDIO   OFF)
set(SDL_VIDEO   ON)
set(SDL_RENDER  OFF)
set(SDL_JOSTICK OFF)
set(SDL_HAPTIC  OFF)
set(SDL_HIDAPI  OFF)
set(SDL_POWER   OFF)
set(SDL_SENSOR  OFF)
# GLM
set(GLM_DISABLE_AUTO_DETECTION  OFF)
set(GLM_ENABLE_LANG_EXTENSIONS  ON)
set(GLM_ENABLE_CXX_20           ON)
set(GLM_ENABLE_FAST_MATH        ON)
set(GLM_FORCE_PURE              OFF)

# fetch vulkan headers alongside glslang
find_package(Vulkan REQUIRED COMPONENTS glslangValidator)
# fetch vulkan-related libraries
include(FetchContent)
set(FETCHCONTENT_QUIET OFF) # enable git output for FetchContent steps
FetchContent_Declare(vk-bootstrap GIT_REPOSITORY "https://github.com/charles-lunarg/vk-bootstrap.git" GIT_TAG "main" GIT_SHALLOW ON)
FetchContent_Declare(spirv_reflect GIT_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Reflect.git" GIT_TAG "main" GIT_SHALLOW ON SOURCE_SUBDIR "disabled/")
FetchContent_Declare(vma GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git" GIT_TAG "master" GIT_SHALLOW ON SOURCE_SUBDIR "disabled/")
FetchContent_Declare(vma_hpp GIT_REPOSITORY "https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git" GIT_TAG "master" GIT_SHALLOW ON GIT_SUBMODULES "" SOURCE_SUBDIR "disabled/")
# fetch misc libraries
FetchContent_Declare(sdl3 GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git" GIT_TAG "main" GIT_SHALLOW ON)
FetchContent_Declare(glm GIT_REPOSITORY "https://github.com/g-truc/glm.git" GIT_TAG "master" GIT_SHALLOW ON)
FetchContent_Declare(fmt GIT_REPOSITORY "https://github.com/fmtlib/fmt.git" GIT_TAG "master" GIT_SHALLOW ON)
FetchContent_Declare(cmrc GIT_REPOSITORY "https://github.com/vector-of-bool/cmrc.git" GIT_TAG "master" GIT_SHALLOW ON)
FetchContent_Declare(imgui GIT_REPOSITORY "https://github.com/ocornut/imgui.git" GIT_TAG "docking" GIT_SHALLOW ON)
FetchContent_Declare(miniaudio GIT_REPOSITORY "https://github.com/mackron/miniaudio.git" GIT_TAG "master" GIT_SHALLOW ON)
FetchContent_MakeAvailable(vk-bootstrap spirv_reflect vma vma_hpp sdl3 fmt cmrc imgui miniaudio)

set(GLM_ENABLE_SIMD_SSE     ${SDL_SSE})
set(GLM_ENABLE_SIMD_SSE2    ${SDL_SSE2})
set(GLM_ENABLE_SIMD_SSE3    ${SDL_SSE3})
set(GLM_ENABLE_SIMD_SSSE3   ${SDL_SSSE3})
set(GLM_ENABLE_SIMD_SSE4_1  ${SDL_SSE4_1})
set(GLM_ENABLE_SIMD_SSE4_2  ${SDL_SSE4_2})
set(GLM_ENABLE_SIMD_AVX     ${SDL_AVX})
set(GLM_ENABLE_SIMD_AVX2    ${SDL_AVX2})
set(GLM_ENABLE_SIMD_AVX512F ${SDL_AVX512F})
FetchContent_MakeAvailable(glm)
set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "faster config speed when enabled" FORCE)

# compile GLSL shaders
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders/")
file(GLOB_RECURSE GLSL_SOURCE_FILES CONFIGURE_DEPENDS 
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp")
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME "${GLSL}" NAME)
    set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        COMMENT "Compiling shader: ${FILE_NAME}"
        OUTPUT  "${SPIRV}"
        COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V "${GLSL}" -o "${SPIRV}"
        DEPENDS "${GLSL}")
    list(APPEND SPIRV_BINARY_FILES "${SPIRV}")
endforeach(GLSL)
add_custom_target(compile-shaders DEPENDS "${SPIRV_BINARY_FILES}")
cmrc_add_resource_library(shaders ALIAS cmrc::shaders WHENCE "${CMAKE_CURRENT_BINARY_DIR}/shaders/" "${SPIRV_BINARY_FILES}")

# gather ImGui sources
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS "${imgui_SOURCE_DIR}/*.cpp")
list(APPEND IMGUI_SOURCES "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp")
list(APPEND IMGUI_SOURCES "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp")

# create executable target
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*")
add_executable(${PROJECT_NAME} 
    "${SOURCE_FILES}" 
    "${IMGUI_SOURCES}"
    "${spirv_reflect_SOURCE_DIR}/spirv_reflect.cpp"
    "${miniaudio_SOURCE_DIR}/extras/miniaudio_split/miniaudio.c")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${imgui_SOURCE_DIR}/"
    "${spirv_reflect_SOURCE_DIR}/"
    "${vma_SOURCE_DIR}/include/"
    "${vma_hpp_SOURCE_DIR}/include/"
    "${miniaudio_SOURCE_DIR}/extras/miniaudio_split/")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "VULKAN_DEBUG_MSG_UTILS"
    "IMGUI_IMPL_VULKAN_NO_PROTOTYPES"
    "VULKAN_HPP_DISPATCH_LOADER_DYNAMIC"
    "VULKAN_HPP_NO_TO_STRING"
    "VULKAN_HPP_NO_SPACESHIP_OPERATOR"
    "VMA_DYNAMIC_VULKAN_FUNCTIONS"
    "VMA_STATIC_VULKAN_FUNCTIONS=0")
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Headers
    vk-bootstrap::vk-bootstrap
    SDL3::SDL3
    glm::glm
    fmt::fmt
    cmrc::shaders
    ${CMAKE_DL_LIBS})