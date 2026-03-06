cmake_minimum_required(VERSION 3.25)
project(Engine LANGUAGES CXX)

# ============================================================================
# C++ 标准
# ============================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# 输出目录 (兼容多配置生成器如 VS)
# ============================================================================
foreach(CONFIG_TYPE Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_SOURCE_DIR}/Bin/${CONFIG_TYPE}-x64/Engine)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_SOURCE_DIR}/Bin/${CONFIG_TYPE}-x64/Engine)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_SOURCE_DIR}/Bin/${CONFIG_TYPE}-x64/Engine)
endforeach()

# ============================================================================
# 查找 vcpkg 依赖
# ============================================================================
find_package(assimp CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(soil CONFIG REQUIRED)
find_package(CompactNSearch CONFIG REQUIRED)

# ============================================================================
# 收集源文件
# ============================================================================
file(GLOB_RECURSE ENGINE_SOURCES
    "Source/*.cpp"
    "Source/*.h"
)

# ============================================================================
# 按目录结构生成 VS 筛选器 (source_group)
# ============================================================================
source_group(TREE ${CMAKE_SOURCE_DIR}/Source PREFIX "Source" FILES ${ENGINE_SOURCES})

# ============================================================================
# 创建可执行目标
# ============================================================================
add_executable(${PROJECT_NAME} ${ENGINE_SOURCES})

# ============================================================================
# 包含目录
# ============================================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/ThirdParty
)

# ============================================================================
# 链接库
# ============================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    assimp::assimp
    GLEW::GLEW
    glfw
    glm::glm
    imgui::imgui
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    soil
    CompactNSearch::CompactNSearch
)

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# ============================================================================
# 预编译头
# ============================================================================
target_precompile_headers(${PROJECT_NAME} PRIVATE Source/pch.h)

# ============================================================================
# 编译定义
# ============================================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

# ============================================================================
# MSVC 特定编译选项
# ============================================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3             # 警告等级 3
        /utf-8          # 源文件和执行字符集为 UTF-8
        /openmp         # 启用 OpenMP
        /sdl            # SDL 安全检查
        /permissive-    # 一致性模式
    )

    # Release 优化选项
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/Gy>
        $<$<CONFIG:Release>:/Oi>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

# ============================================================================
# 控制台子系统 (Windows) + VS 调试工作目录
# ============================================================================
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()
