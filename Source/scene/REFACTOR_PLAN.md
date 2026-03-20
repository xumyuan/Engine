# Scene3D 解构重构计划

## 背景
当前 `Scene3D` 类承担了过多职责（场景数据存储、JSON加载、渲染提交、逻辑更新、渲染状态初始化），
违反单一职责原则，不利于维护和扩展。本计划分4个阶段渐进式解构。

---

## ✅ 阶段一：抽取 SceneLoader（分离加载逻辑） —— 已完成

**目标**：将 `Scene3D::init()` 中的 JSON 解析和对象创建逻辑抽取到独立的 `SceneLoader` 类。

### 完成内容
- 新增 `scene/SceneLoader.h` 和 `scene/SceneLoader.cpp`
- `SceneLoader::loadFromFile()` 负责 JSON 解析
- `SceneLoader::loadModels()` / `loadSkybox()` / `loadLights()` 分职责加载
- `Scene3D` 新增 `addRenderableModel()` 和 `setSkybox()` 数据注入接口
- `Scene3D::init()` 已移除，构造函数调用 `SceneLoader::loadFromFile()`

---

## ✅ 阶段二：引入 RenderScene，解耦渲染系统 —— 已完成

**目标**：RenderPass 不再直接持有 `Scene3D*`，改为接收轻量只读数据结构 `RenderScene`。

### 完成内容
- 新增 `scene/RenderScene.h` 和 `scene/RenderScene.cpp`
- `RenderScene` 结构体包含所有渲染数据指针 + `submitModelsToRenderer()` 方法
- `Scene3D` 新增 `extractRenderScene()` 方法
- `RenderPass` 基类 `Scene3D* m_ActiveScene` → `RenderScene m_RenderScene`
- `MasterRenderer` 每帧 extract 并传递 `RenderScene`
- 所有 7 个 RenderPass 子类已改用 `m_RenderScene.xxx` 访问数据

---

## 阶段三：统一 SceneNode 模型（组件化）

**目标**：所有场景实体统一为 `SceneNode`，通过 Component 系统区分能力。

### 具体任务
1. **新增 `scene/Component.h`** — 组件基类
2. **新增 `scene/SceneNode.h/cpp`** — 场景节点（Transform + 父子关系 + Component 容器）
3. **新增 `scene/components/MeshComponent.h`** — 模型渲染组件
4. **新增 `scene/components/LightComponent.h`** — 灯光组件
5. **新增 `scene/components/SkyboxComponent.h`** — 天空盒组件
6. **新增 `scene/components/TerrainComponent.h`** — 地形组件
7. **修改 `Scene3D`** — 内部存储改为 `SceneNode m_Root` 节点树
8. **修改 `SceneLoader`** — 创建 SceneNode + 对应 Component
9. **修改 `RenderScene` 提取逻辑** — 遍历节点树收集渲染数据
10. **废弃 `RenderableModel`** — 拆分为 SceneNode + MeshComponent

### 受影响的文件
- 新增 6+ 文件
- 修改 5+ 文件

### 风险：高（改动面大，需要充分测试）

---

## ✅ 阶段四：引入 SceneManager（场景生命周期管理） —— 已完成

**目标**：支持多场景管理和场景切换。

### 完成内容
- 新增 `scene/SceneManager.h` 和 `scene/SceneManager.cpp`
- `SceneManager::createScene()`, `destroyScene()`, `setActiveScene()`, `getActiveScene()`
- `SceneManager::loadScene()` — 创建场景 + SceneLoader 加载
- `main.cpp` 改为使用 `SceneManager` 管理场景

---

## 执行顺序
~~阶段一 → 阶段四 → 阶段二 → 阶段三~~
实际顺序：阶段一 → 阶段二 → 阶段四 → 阶段三（待做）
