# LunarRender 通用渲染运行时重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 将 LunarRender 从 MoonMC 专用 OpenGL 客户端重构为不依赖 MoonMC 的通用渲染运行时，并保留可运行的 OpenGL 示例后端。

**Architecture:** `renderer` 只提供纯 MoonBit 渲染数据；`backend/opengl` 提供 OpenGL 3.3 实现；`platform/native` 提供窗口、输入、时间和文件平台接口；`cmd/lunarrender` 只运行静态渲染示例。MoonMC 的游戏规则、玩家、HUD、物品栏和命令不进入 LunarRender。

**Tech Stack:** MoonBit native, Windows MSVC, CMake, GLFW 3.4, GLAD 2.0.8, OpenGL 3.3 Core, PowerShell。

**Spec:** `docs/superpowers/specs/2026-09-24-lunarrender-runtime-design.md`

## Global Constraints

- LunarRender 模块固定为 `PingGuoMiaoMiao/LunarRender@0.1.0`。
- `moon.mod` 不导入 `PingGuoMiaoMiao/MoonMC`。
- `renderer` 不导入 MoonMC、平台包或 C FFI。
- OpenGL C FFI 只位于 `backend/opengl`，符号使用 `lunarrender_opengl_*`。
- 窗口平台 FFI 只位于 `platform/native`，符号使用 `lunarrender_platform_*`。
- 默认构建不包含 Steve、Minecraft 方块、HUD、存档、资源包切换和私有 MMD 文件。
- OpenGL 后端第一版接受 6 个 Float/顶点：位置 3 个、UV 2 个、明暗 1 个。
- 现有主分支和原 MoonMC 工作区不重置、不清理、不覆盖。
- 不执行 Mooncakes 发布；GitHub 推送只在本计划的最终验证通过后执行。

## Review Focus

- MoonMC 依赖残留：检查模块、包配置、源码和文档中的直接导入。
- 游戏逻辑残留：检查示例程序是否仍包含区块、玩家、HUD、存档或命令调用。
- FFI 归属错误：检查平台符号和 OpenGL 符号是否进入了错误的包。
- 顶点格式错误：检查 6 Float/顶点的长度、步幅和空数据上传。
- 生命周期错误：检查 context 未初始化、重复销毁和 window 销毁后绘制。

---

### Task 1: 固定通用 renderer 数据接口

**Files:**
- Create: `renderer/render_types.mbt`
- Create: `renderer/render_types_wbtest.mbt`
- Modify: `renderer/moon.pkg`
- Test: `renderer/render_types_wbtest.mbt`

**Interfaces:**
- Produces `@renderer.MeshData`, `@renderer.FrameData`, `MeshData::valid` and `FrameData::valid`.
- `MeshData::valid` requires non-negative vertex count, six Float values per vertex and exact array length.
- `FrameData::valid` requires exactly 16 matrix values.

- [ ] Write tests for valid mesh data, empty mesh data, wrong stride, wrong array length and matrix length.
- [ ] Run `moon test --target native renderer` and observe failure because the types and validators do not exist.
- [ ] Implement the two public structs and validators without importing MoonMC or native packages.
- [ ] Run `moon test --target native renderer` and require all renderer tests to pass.
- [ ] Run `rg "PingGuoMiaoMiao/MoonMC|extern \"C\"|platform/native" renderer` and require no matches.
- [ ] Commit with `feat: add backend-neutral render data`.

### Task 2: Move OpenGL implementation into a backend package

**Files:**
- Create: `backend/opengl/opengl.mbt`
- Create: `backend/opengl/moon.pkg`
- Create: `backend/opengl/opengl_wbtest.mbt`
- Move/Modify: `platform/native/platform.mbt` → retain only platform declarations
- Move/Modify: `platform/native/platform.c` → split window code from OpenGL code
- Modify: `platform/native/moon.pkg`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces `@opengl.Renderer::new`, `upload_mesh`, `remove_mesh`, `draw_frame` and `shutdown`.
- Consumes `@renderer.MeshData` and `@renderer.FrameData`.
- OpenGL C symbols become `lunarrender_opengl_*`; platform window symbols become `lunarrender_platform_*`.

- [ ] Add backend tests for a rejected invalid mesh and a renderer object with zero uploaded meshes.
- [ ] Run the backend tests before implementation and observe missing package/API failures.
- [ ] Extract OpenGL declarations and C implementation into `backend/opengl`; keep window, key, mouse, time and file declarations in `platform/native`.
- [ ] Replace chunk/player/HUD-specific GPU functions with keyed generic mesh storage.
- [ ] Keep one OpenGL vertex layout: six Float values per vertex, with position, UV and shade attributes.
- [ ] Update CMake and MoonBit link flags to compile the split files and link GLFW/GLAD/OpenGL.
- [ ] Run `cmake -S . -B third_party/build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF`.
- [ ] Run `cmake --build third_party/build --config Release`.
- [ ] Run `moon test --target native backend/opengl` and require the package tests to pass.
- [ ] Commit with `refactor: isolate OpenGL backend`.

### Task 3: Replace the game client with a renderer-only example

**Files:**
- Modify: `cmd/lunarrender/main.mbt`
- Modify: `cmd/lunarrender/main_wbtest.mbt`
- Modify: `cmd/lunarrender/moon.pkg`
- Modify: `scripts/build.ps1`
- Modify: `scripts/capture-runtime-baseline.ps1`
- Remove from default runtime: Minecraft world, player, physics, HUD, inventory, command, save and resource-pack calls.

**Interfaces:**
- Consumes `@renderer.FrameData`, `@renderer.MeshData`, `@opengl.Renderer` and `@native` platform functions.
- Produces a static triangle or cube example with no MoonMC import.

- [ ] Add a white-box test asserting the example uses a 16-value identity matrix and a six-float-per-vertex mesh.
- [ ] Run the client test before implementation and observe imports/API failures after the generic package boundary is applied.
- [ ] Implement window creation, backend initialization, one static mesh upload, frame loop, Escape exit and reverse-order shutdown.
- [ ] Remove the MoonMC dependency from `cmd/lunarrender/moon.pkg` and `moon.mod`.
- [ ] Remove Minecraft-specific runtime asset generation from the default build script.
- [ ] Run `moon check --target native` and `moon test --target native`.
- [ ] Run `moon build --target native --release cmd/lunarrender`.
- [ ] Start the executable and require logs for window/context initialization, mesh upload, frame loop start and renderer/window shutdown.
- [ ] Commit with `refactor: make LunarRender independent from MoonMC`.

### Task 4: Isolate optional model-format support

**Files:**
- Move: `core/mmd` → `formats/mmd`
- Modify: `formats/mmd/moon.pkg`
- Modify: `.gitignore`
- Modify: `AGENT.md`, `ARCHITECTURE.md`, `FFI_BINDINGS.md`, `README.md`, `ROADMAP.md`

**Interfaces:**
- Produces optional PMX parsing and generic model data only.
- Does not import MoonMC, OpenGL FFI, private model files or generated model C files.

- [ ] Run the existing MMD tests before moving the package and record the baseline.
- [ ] Move the package without changing parser behavior.
- [ ] Run `moon test --target native formats/mmd` and require the existing parser tests to pass.
- [ ] Search tracked files for PMX, PMD, VMD, private paths and generated C files; require no private assets.
- [ ] Commit with `refactor: isolate optional model formats`.

### Task 5: Write application materials and final documentation

**Files:**
- Create: `PROJECT_PROPOSAL.md`
- Modify: `README.md`, `AGENT.md`, `ARCHITECTURE.md`, `FFI_BINDINGS.md`, `ROADMAP.md`

**Interfaces:**
- Documentation must describe LunarRender as a cross-backend MoonBit rendering runtime.
- The application must clearly list OpenGL as the first implementation backend, not as the project identity.
- The application must exclude game-owned features from LunarRender's ownership list.

- [ ] Check every document for contradictory statements that call LunarRender an OpenGL-only game client.
- [ ] Add project background, objectives, architecture, innovation, technical route, MoonBit contribution, schedule, deliverables, risks and references to `PROJECT_PROPOSAL.md`.
- [ ] Run `rg "Steve|物品栏|命令行|重力|碰撞|方块破坏|OpenGL 客户端" AGENT.md ARCHITECTURE.md README.md ROADMAP.md PROJECT_PROPOSAL.md` and review each match for ownership accuracy.
- [ ] Commit with `docs: define LunarRender runtime proposal`.

### Task 6: Full validation and GitHub delivery

**Files:**
- Verify: all tracked source, docs and build files.
- Modify: none unless a verification failure requires a focused fix.

- [ ] Run `git diff --check`.
- [ ] Run `moon check --target native`.
- [ ] Run `moon test --target native`.
- [ ] Run the CMake configure and Release build commands from Task 2.
- [ ] Run `moon build --target native --release cmd/lunarrender`.
- [ ] Launch the renderer example and inspect the fresh runtime log.
- [ ] Run `git status --short --branch` and require a clean worktree.
- [ ] Fast-forward the original LunarRender `main` to the verified branch without resetting files.
- [ ] Run `git push -u origin main` from the clean original LunarRender repository.
- [ ] Verify the remote branch and commit using `git ls-remote origin refs/heads/main`.

