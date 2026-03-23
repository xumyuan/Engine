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

## ✅ 阶段三：统一 SceneNode 模型（组件化） —— 已完成

**目标**：所有场景实体统一为 `SceneNode`，通过 Component 系统区分能力。

### 完成内容
- 新增 `scene/Component.h` — 组件基类（ComponentType 枚举 + Component 抽象接口）
- 新增 `scene/SceneNode.h/cpp` — 场景节点（Transform + 父子层级 + Component 容器 + 模板化组件查询）
- 新增 `scene/components/MeshComponent.h` — 模型渲染组件（替代 RenderableModel 的渲染职责）
- 新增 `scene/components/LightComponent.h` — 灯光组件（支持方向光/点光源/聚光灯）
- 新增 `scene/components/SkyboxComponent.h` — 天空盒组件
- 新增 `scene/components/TerrainComponent.h` — 地形组件
- `Scene3D` 新增 `SceneNode m_RootNode` 节点树 + `addSceneNode()` / `getRoot()` 接口
- `SceneLoader` 加载时同时创建 SceneNode + Component 并添加到节点树
- `RenderScene` 新增 `SceneNode* rootNode` 指针，支持遍历节点树
- `RenderableModel` 暂时保留（向后兼容），后续逐步废弃

### 向后兼容说明
- 渲染管线仍通过 `RenderableModel` + `ModelRenderer` 工作（不破坏现有渲染流程）
- 节点树系统与旧系统并行存在，可通过 `RenderScene::rootNode` 遍历场景
- 后续可逐步将渲染管线切换为直接从节点树 + MeshComponent 获取数据

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
实际顺序：阶段一 → 阶段二 → 阶段四 → 阶段三

**全部阶段已完成！** 🎉

---

## ✅ 后续优化：灯光系统迁移 — DynamicLightManager → LightCollector

**目标**：用 LightComponent + LightCollector 替代旧的 DynamicLightManager，实现标准的三层灯光架构。

### 完成内容
- 新增 `scene/LightCollector.h/cpp` — 灯光收集器
  - `fillLightingUBO(rootNode, ubo)` 遍历节点树收集 LightComponent → 填充 UBOLighting
  - `getDirectionalLightDirection(rootNode)` 获取方向光方向（供阴影图使用）
  - `findNodeByName(rootNode, name)` 按名称查找节点（供运行时控制使用）
- `RenderScene.h` — `DynamicLightManager* lightManager` → `LightCollector* lightCollector`
- `Scene3D.h/cpp` — `DynamicLightManager` → `LightCollector`，聚光灯跟随相机改为通过节点树操作
- `SceneLoader.cpp` — 移除 DynamicLightManager 双写逻辑，灯光仅通过 LightComponent 创建
- `DeferredLightingPass.cpp`、`ForwardLightingPass.cpp`、`ShadowmapPass.cpp` — 改用 LightCollector

### 架构变更
```
场景层：LightComponent（数据持有者）
收集层：LightCollector（遍历节点树 → 填充 UBO）
GPU层：UBOLighting（结构体不变，Shader 不变）
```

### ✅ 已删除的旧文件
- `graphics/dynamic_lights/` 整个目录已删除（含 DynamicLightManager、DynamicLight、DirectionalLight、SpotLight、PointLight）
