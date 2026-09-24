# LunarRender 路线图

## 阶段一：通用边界基线（已完成）

内容：

- `renderer` 纯 MoonBit 网格和帧数据。
- `platform/native` 与 `backend/opengl` 分离。
- 取消对 MoonMC 的模块依赖。
- 将 PMX 解析移到 `formats/mmd`。
- 建立 native 测试、CMake 构建和运行日志。

验收：

- `moon.mod` 不导入 MoonMC。
- `renderer` 不声明 FFI。
- OpenGL 和平台 C 符号分别使用各自前缀。
- 示例可以创建窗口、上传网格、绘制并按逆序销毁。

## 阶段二：渲染基础扩展（已完成）

内容：

- `MeshHandle`、`TextureHandle` 和 `MaterialHandle` 三类独立资源句柄。
- 严格 RGBA8 纹理数据、tint、`Opaque`/`Blend` 材质。
- framebuffer 尺寸查询和动态 `glViewport`。
- 深度测试、透明混合、纹理采样和 36 顶点旋转立方体示例。
- native 测试、Release 构建和真实窗口 `--self-test` 日志验收。

验收：

- 新增资源类型没有引入游戏字段，句柄类型不能交叉传递。
- RGBA 长度、材质引用、网格引用和 framebuffer 尺寸均有校验。
- OpenGL 日志确认纹理、材质和网格上传及逆序销毁。
- `.\_build\native\release\build\cmd\lunarrender\lunarrender.exe --self-test` 退出码为 0。

## 阶段三：游戏组合层接入（后续）

内容：

- 单独的游戏客户端组合项目。
- 将 MoonMC 世界和区块网格转换成 `MeshData`。
- 将相机和选择状态转换成 `FrameData` 及应用层附加数据。
- Steve、第一/第三人称、重力、碰撞、跳跃、HUD、物品栏和命令由游戏层实现。

验收：

- LunarRender 不增加 MoonMC 依赖。
- 游戏层可以替换渲染后端而不修改世界和物理代码。

## 阶段四：模型和材质扩展

内容：

- PMX 数据转换为通用模型资源。
- 材质、法线、骨骼和动画数据的独立格式。
- 私有模型只通过本地准备脚本加载。

验收：

- 没有私有资源时默认构建仍通过。
- 模型格式解析与 GPU 上传之间存在明确数据边界。

## 阶段五：多后端评估

顺序：

1. 评估软件后端，用于无 GPU 测试。
2. 根据公共接口实际限制评估 Vulkan、WebGPU 或其他后端。
3. 记录每个后端支持的顶点格式、纹理和同步能力。

验收：

- 后端差异记录在能力文档中。
- 不为单个后端临时增加不可解释的游戏字段。

## 不在本仓库默认路线中的内容

- Minecraft 游戏规则本体。
- 无限区块、噪声地形、物理和联网。
- Steve 角色语义和游戏命令。
- 私有 MMD 资源和生成的 C 文件。
