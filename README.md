# Engine

## 依赖

项目采用 [vcpkg](https://github.com/microsoft/vcpkg) 管理依赖，使用 manifest 模式（`vcpkg.json`），配置和编译时会自动安装依赖。

**前置要求：**
- CMake 3.25+
- Visual Studio 2022（MSVC v143）
- vcpkg（需设置 `VCPKG_ROOT` 环境变量）

## 构建

```bash
# 配置（首次运行）
cmake --preset default

# 编译
cmake --build --preset debug      # Debug
cmake --build --preset release    # Release
```

也可以直接用 Visual Studio 打开 `build/Engine.sln` 进行开发调试。