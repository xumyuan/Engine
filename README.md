# Engine

一个基于 OpenGL 的 3D 渲染引擎，支持场景管理、模型加载、地形渲染和 ImGui 调试界面。

## 依赖

项目采用 [vcpkg](https://github.com/microsoft/vcpkg) 管理依赖，使用 manifest 模式（`vcpkg.json`），配置和编译时会自动安装依赖。

**前置要求：**
- CMake 3.25+
- Windows x64，Visual Studio 2022（安装“使用 C++ 的桌面开发”、MSVC v143 和 Windows SDK）
- vcpkg（需设置 `VCPKG_ROOT` 环境变量）
- clangd 支持额外需要 Ninja 在 `PATH` 中，并使用 VS2022 的 **x64 Native Tools Command Prompt**；编辑器需安装 clangd

## 构建

### 快速开始

以下脚本命令在 Windows CMD 中执行；PowerShell 中使用 `./configure.bat`。

```bat
:: 配置 VS2022 工程
configure.bat

:: 同时生成 clangd 编译数据库（需 x64 Native Tools Command Prompt）
configure.bat --clangd

:: 重置 CMake 缓存后重新配置
configure.bat --clean
```

`--clean` 使用 CMake 的 `--fresh`，重置所请求 preset 的 CMake 缓存和 `CMakeFiles`，不删除整个输出目录或已安装的依赖。组合使用 `--clean --clangd` 会重置两个 preset。配置失败时脚本返回非零退出码。

### 配置与编译

```bash
cmake --preset default            # 配置 VS2022 工程
cmake --build --preset debug      # 编译 Debug
cmake --build --preset release    # 编译 Release
cmake --build --preset relwithdebinfo  # 编译带调试符号的优化版本
```

也可以使用 workflow 一条命令完成配置和编译，本地与 CI 均使用相同入口：

```bash
cmake --workflow --preset debug
cmake --workflow --preset release
cmake --workflow --preset relwithdebinfo
```

VS 是多配置生成器，三种构建类型共享 `out/build/default` 的工程缓存，产物分别位于 `bin/Debug`、`bin/Release`、`bin/RelWithDebInfo`；静态库和导入库位于对应的 `lib/<配置>`。可直接打开 `out/build/default/Engine.sln` 开发调试。

### clangd

在 **x64 Native Tools Command Prompt for VS 2022** 中执行：

```bash
cmake --preset clangd
# 可选：用相同 Ninja 配置编译
cmake --build --preset clangd
```

`clangd` preset 使用 Ninja + MSVC 的 Debug 配置，编译器是 `cl`，并不切换为 Clang。它在 `out/build/clangd` 中生成 `compile_commands.json`，仓库的 `.clangd` 已指向此目录。为避免 clangd 读取 MSVC 预编译头，此 preset 关闭 PCH；VS preset 仍启用 PCH。

两个 configure preset 的缓存、产物和 vcpkg 安装目录相互隔离，依赖版本和 overlay ports 统一由仓库的 vcpkg manifest/configuration 管理。上述 presets 仅在 Windows 可用；切换编译器或工具链时应重新配置缓存。

### 旧构建目录迁移

重新运行配置命令即可使用新目录；旧的 `Build/`、`Build-clangd/`、`Bin/` 和根目录 `vcpkg_installed/` 不会被脚本删除或继续用作这些 presets 的输出目录。已有 IDE 配置或启动脚本需要更新可执行文件路径，例如 `out/build/default/bin/Debug/Engine.exe`。

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

## 流体模拟后端

场景 JSON 的 `fluid.backend` 可设为 `"cpu"`（默认，后台线程 PBF）或 `"compute"`（GPU PBF）。
`Assets/scene/sponza_fluid.json` 已启用 compute，要求 OpenGL 4.3+；设备不支持、资源规模超限或着色器编译失败时会记录警告并回退到 CPU。

GPU 路径使用固定 0.01 秒时间步，每帧最多追赶 4 步。预测、网格邻域搜索、3 轮密度约束与位置修正、XSPH 粘性全部在 GPU 上执行；位置缓冲直接用于前向粒子和屏幕空间流体绘制，没有逐帧 CPU 回读或位置上传。`autoStart`、`startSimulation()` 和 `stopSimulation()` 同样适用。

compute 参数在创建时固定，CPU 的 `getPositions()` / `getVelocities()` / `getNeighborList()` 不反映 GPU 当前状态。GPU 每轮应用修正并重建邻域、修正后限制边界，因此与现有 CPU 求解器不保证逐粒子数值一致。macOS 原生 OpenGL 最高为 4.1，无法运行 compute 路径；当前引擎窗口本身要求 OpenGL 4.5，需在支持该版本的环境验证运行效果。

## RHI 测试

```bash
# 构建后运行 NullDevice 测试
Engine.exe --test-rhi
```

不需要图形上下文的 compute 命令与资源测试（仅依赖 GLM）：

```bash
cmake -S Tests -B out/build/compute-tests -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build out/build/compute-tests --config Debug
ctest --test-dir out/build/compute-tests -C Debug --output-on-failure
```

测试覆盖即时/延迟命令一致性、每轮网格重建与屏障、非整工作组粒子数以及初始化失败后的资源释放；不替代真实 GPU 上的数值和视觉验证。
