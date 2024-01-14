cmake_minimum_required(VERSION 3.28)
project(vulkan-renderer LANGUAGES CXX)

# Generic CMake Settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared instead of static libraries" FORCE)
# check for IPO/LTO support
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW CACHE STRING "Set policy on IPO/LTO" FORCE)
include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_ENABLED LANGUAGES CXX)
message("IPO/LTO enabled: ${LTO_ENABLED}")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION  ${LTO_ENABLED} CACHE BOOL "Build with Link-time Optimization by default" FORCE)

# SDL Build settings
set(SDL_SHARED              ON  CACHE BOOL "Build SDL as shared lib" FORCE)
set(SDL_STATIC              OFF CACHE BOOL "Build SDL as static lib" FORCE)
set(SDL_TEST_LIBRARY        OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_DISABLE_INSTALL     ON  CACHE BOOL "Disable installation   of SDL3" FORCE)
set(SDL_DISABLE_UNINSTALL   ON  CACHE BOOL "Disable uninstallation of SDL3" FORCE)
# SDL Graphics API support
set(SDL_VULKAN              ON  CACHE BOOL "Enable Vulkan support" FORCE)
set(SDL_DIRECTX             OFF CACHE BOOL "Enable DirectX support" FORCE)
set(SDL_OPENGL              OFF CACHE BOOL "Enable OpenGL support" FORCE)
set(SDL_OPENGLES            OFF CACHE BOOL "Enable OpenGL ES support" FORCE)
# SDL Subsystems
set(SDL_ATOMIC              OFF CACHE BOOL "Disable SDL Atomic subsystem" FORCE)
set(SDL_AUDIO               OFF CACHE BOOL "Disable SDL Audio subsystem" FORCE)
set(SDL_VIDEO               ON  CACHE BOOL "Disable SDL Video subsystem" FORCE)
set(SDL_RENDER              OFF CACHE BOOL "Disable SDL Render subsystem" FORCE)
set(SDL_EVENTS              ON  CACHE BOOL "Disable SDL Events subsystem" FORCE)
set(SDL_JOSTICK             OFF CACHE BOOL "Disable SDL Joystick subsystem" FORCE)
set(SDL_HAPTIC              OFF CACHE BOOL "Disable SDL Haptic subsystem" FORCE)
set(SDL_HIDAPI              ON  CACHE BOOL "Disable SDL Hidapi subsystem" FORCE)
set(SDL_POWER               OFF CACHE BOOL "Disable SDL Power subsystem" FORCE)
set(SDL_THREADS             ON  CACHE BOOL "Disable SDL Threads subsystem" FORCE)
set(SDL_TIMERS              OFF CACHE BOOL "Disable SDL Timers subsystem" FORCE)
set(SDL_FILE                ON  CACHE BOOL "Disable SDL File subsystem" FORCE)
set(SDL_LOADSO              ON  CACHE BOOL "Disable SDL Loadso subsystem" FORCE)
set(SDL_CPUINFO             OFF CACHE BOOL "Disable SDL CPUinfo subsystem" FORCE)
set(SDL_FILESYSTEM          ON  CACHE BOOL "Disable SDL Filesystem subsystem" FORCE)
set(SDL_SENSOR              OFF CACHE BOOL "Disable SDL Sensor subsystem" FORCE)
set(SDL_LOCALE              ON  CACHE BOOL "Disable SDL Locale subsystem" FORCE)
set(SDL_MISC                ON  CACHE BOOL "Disable SDL Misc subsystem" FORCE)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF) # enable git output for FetchContent steps
find_package(Vulkan REQUIRED COMPONENTS glslc glslangValidator glslang shaderc_combined) # TODO: check which glsl compiler we need (glsl -> spir-v -> shader bytecode)
FetchContent_Declare(sdl3 GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git" GIT_TAG "main" GIT_SHALLOW ON)
FetchContent_Declare(glm GIT_REPOSITORY "https://github.com/g-truc/glm.git" GIT_TAG "master" GIT_SHALLOW ON)
FetchContent_Declare(fmt GIT_REPOSITORY "https://github.com/fmtlib/fmt.git" GIT_TAG "master" GIT_SHALLOW ON)
FetchContent_Declare(vk-bootstrap GIT_REPOSITORY "https://github.com/charles-lunarg/vk-bootstrap.git" GIT_TAG "main" GIT_SHALLOW ON)
FetchContent_Declare(vma GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git" GIT_TAG "master" GIT_SHALLOW ON)
FetchContent_Declare(vma-hpp GIT_REPOSITORY "https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git" GIT_TAG "master" GIT_SHALLOW ON GIT_SUBMODULES "" SOURCE_SUBDIR "include/")
FetchContent_MakeAvailable(sdl3 glm fmt vk-bootstrap vma vma-hpp)
set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "faster config speed when enabled" FORCE)

# create executable target
file(GLOB_RECURSE source-files CONFIGURE_DEPENDS "src/*")
add_executable(${PROJECT_NAME} "${source-files}")
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${vma-hpp_SOURCE_DIR}/include/")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VULKAN_DEBUG_MSG_UTILS
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    VULKAN_HPP_NO_TO_STRING
    VULKAN_HPP_NO_SPACESHIP_OPERATOR
    VMA_DYNAMIC_VULKAN_FUNCTIONS
    VMA_STATIC_VULKAN_FUNCTIONS=0)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Headers
    SDL3::SDL3
    glm::glm
    fmt::fmt
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
    ${CMAKE_DL_LIBS})