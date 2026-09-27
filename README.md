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
cmake -S Tests -B Build/compute-tests -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build Build/compute-tests --config Debug
ctest --test-dir Build/compute-tests -C Debug --output-on-failure
```

测试覆盖即时/延迟命令一致性、每轮网格重建与屏障、非整工作组粒子数以及初始化失败后的资源释放；不替代真实 GPU 上的数值和视觉验证。
