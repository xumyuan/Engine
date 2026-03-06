# Engine

一个基于 OpenGL 的 3D 渲染引擎，支持场景管理、模型加载、地形渲染和 ImGui 调试界面。

## 依赖

项目采用 [vcpkg](https://github.com/microsoft/vcpkg) 管理依赖，使用 manifest 模式（`vcpkg.json`），配置和编译时会自动安装依赖。

**前置要求：**
- CMake 3.25+
- Visual Studio 2022（MSVC v143）
- vcpkg（需设置 `VCPKG_ROOT` 环境变量）

## 构建

### 快速开始

```bash
# 一键配置（推荐）
configure.bat

# 配置并生成 clangd 支持  需要配置插件
configure.bat --clangd

# 清除缓存后重新配置
configure.bat --clean
```

### 手动配置

```bash
cmake --preset default            # 配置 VS2022 工程
cmake --build --preset debug      # 编译 Debug
cmake --build --preset release    # 编译 Release
```

配置完成后可直接用 Visual Studio 打开 `Build/Engine.sln` 进行开发调试。

## 项目结构

```
Engine/
├── Assets/          # 纹理、模型等资源文件
├── Shaders/         # GLSL 着色器
├── Source/           # 源代码
│   ├── graphics/     # 渲染、窗口、相机、Shader
│   ├── rhi/          # 渲染硬件接口抽象层
│   │   ├── include/  # RHI 公共接口
│   │   ├── null/     # Null 后端（测试用）
│   │   ├── opengl/   # OpenGL 后端（开发中）
│   │   └── dx12/     # D3D12 后端（规划中）
│   ├── scene/        # 场景管理
│   ├── terrain/      # 地形系统
│   ├── ui/           # ImGui 面板
│   └── utils/        # 工具类
├── ThirdParty/       # 第三方头文件
├── configure.bat     # 一键配置脚本
└── CMakePresets.json # CMake 预设
```

## RHI 测试

```bash
# 构建后运行 NullDevice 测试
Engine.exe --test-rhi
```