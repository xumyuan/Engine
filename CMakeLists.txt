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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib/$<CONFIG>")

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
# CONFIGURE_DEPENDS：每次构建时重新检查文件列表，新增 / 删除源文件后无需手动重新配置
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "Source/*.cpp"
    "Source/*.h"
)
set(ENGINE_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders")
file(GLOB_RECURSE ENGINE_SHADERS CONFIGURE_DEPENDS
    "${ENGINE_SHADER_SOURCE_DIR}/*.glsl"
)
set_source_files_properties(${ENGINE_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

# ============================================================================
# 按目录结构生成 VS 筛选器 (source_group)
# ============================================================================
source_group(TREE ${CMAKE_SOURCE_DIR}/Source PREFIX "Source" FILES ${ENGINE_SOURCES} ${ENGINE_SHADERS})

# ============================================================================
# 创建可执行目标
# ============================================================================
add_executable(${PROJECT_NAME} ${ENGINE_SOURCES} ${ENGINE_SHADERS})

# Synchronize shaders even when no C++ files need rebuilding.
add_custom_target(${PROJECT_NAME}Shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ENGINE_SHADER_SOURCE_DIR}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders"
    DEPENDS ${ENGINE_SHADERS}
    COMMENT "Synchronizing shaders to the runtime directory"
    VERBATIM
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}Shaders)

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
# 以 STB_IMAGE_STATIC 编译线程安全的 stb_image，不能与 pch.h 中的外部链接声明同处一个编译单元
set_source_files_properties(Source/utils/loaders/ImageDecoder.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

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
