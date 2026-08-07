# N_m3u8DL-RE-GUI-Qt

基于 **Qt 6** 的 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 图形界面。
将 `doc/N_m3u8DL-RE_README.md` 中记录的命令行参数以侧边栏分组导航的形式呈现，并负责拼装命令行、调用
`N_m3u8DL-RE` 可执行文件、实时显示输出日志。

> 本项目是 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 的 **Qt 图形前端**，
> 自身不实现下载逻辑，所有实际的下载 / 解密 / 混流能力均来自该上游命令行工具。

## 功能

**核心体验**
- 🚀 **URL 置顶输入框** — 打开即用，粘贴链接 → 回车下载，主流程极简。
- ⚡ **实时命令预览** — 修改任意参数即刻生成完整命令行，一键复制到终端。
- 📎 **剪贴板自动填充** — 复制链接自动填入并派生保存文件名，双击文件名可重新派生。

**下载管理**
- 📥 **并发限制 + 等待队列** — 可限制同时下载数量，超额任务自动排队，完成后自动续传。
- 🖥️ **独立终端模式** — 切换后在单独终端窗口中运行，适合需要交互式输出的场景，
  同样受并发限制约束。
- 📋 **下载列表 + 右键菜单** — 列表记录所有任务及状态（运行中 · 等待中 · 已完成 · 异常退出），
  右键可停止单个任务、取消等待、复制链接。

**参数设置**
- 🗂️ **侧边栏分类导航** — 基本设置 / 网络 / 音视频 / 后处理 / 直播 / 高级，
  每个分类内再按「目录与命名」「下载与超时」等小组组织，切换即用。
- 🎛️ **运行控制** — 并发限制与独立终端统一收纳在「基本设置」分类页。
- 🔁 **可重复项支持** — 请求头 `-H`、解密密钥 `--key`、外部媒体 `--mux-import` 等支持多项添加。

**日志与监控**
- 📜 **实时日志 + 按任务筛选** — stdout/stderr 逐条带 `[#id]` 前缀，长行自动换行，
  点击下载列表项即按该任务筛选日志。
- 📋 **右键菜单** — 清除日志 / 复制全部日志 / 复制当前任务可见日志。

**系统与环境**
- 🔍 **自动探测** — 程序目录 → `third_party/` → `PATH` 中搜索 exe/ffmpeg，命中后绿色提示。
- 🎨 **主题切换** — 跟随系统 / 浅色 / 暗色，置顶开关，状态持久化。
- 📦 **便携模式** — 配置文件 `config.ini` 保存在程序目录，拷贝即迁移。
- 🔄 **恢复默认** — 一键重置所有配置到出厂状态。

## 界面布局

```
┌──────────────────────────────────────────────────────┐
│ 程序:   [exe 路径] [浏览...]                          │
│ ffmpeg: [路径] [浏览...]                              │ ← 输入控制区
│ 链接:   [URL] [▶ 开始下载]                            │   （label 对齐）
│ 保存文件名: [________]                                │
│ [■ 停止所有] [📂 打开输出目录]   运行中:0 等待中:0     │
├──────────────────────────────────────────────────────┤
│   [ 下载列表 | 参数设置 ]          ← 顶层页签          │
├──────────────────────────────────────────────────────┤
│ ┌ 下载列表页 ──────────────────────────────────────┐ │
│ │  下载列表 │ 日志  (QSplitter)                     │ │
│ └──────────────────────────────────────────────────┘ │
│ ┌ 参数设置页 ──────────────────────────────────────┐ │
│ │  运行控制: 并发 限制[5] 独立终端                   │ │
│ │  ┌─侧边栏─┐ ┌─分类内容─────────────────┐          │ │
│ │  │基本设置 │ │ 分组标题                  │          │ │
│ │  │网络     │ │ 参数表单（两列/整行）      │          │ │
│ │  │音视频   │ │ …                        │          │ │
│ │  │后处理   │ └──────────────────────────┘          │ │
│ │  │直播     │  命令预览: [多行命令] [复制]            │ │
│ │  │高级     │                                        │ │
│ │  └─────────┘                                       │ │
│ └──────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

## 快速开始

1. 准备 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 的可执行文件（`N_m3u8DL-RE.exe`），
   以及可选的 `ffmpeg.exe`（用于合并 / 解密）。
2. 将 `N_m3u8DL-RE.exe` 放到 GUI 程序所在目录或 `third_party/` 下（或确保其位于 `PATH`），
   `ffmpeg.exe` 同理；程序启动后会**自动探测**并显示绿色提示。
3. 复制一个 m3u8 / dash 链接，程序会自动填入「链接」并派生「保存文件名」。
4. 在「参数设置」页的侧边栏分类中按需调整参数，命令预览会实时更新。
5. 点击「开始下载」或按 Enter。在「下载列表」页监控进度，点击任务名筛选对应日志。

## 构建

环境要求：

- Qt 6.11+（llvm-mingw_64 或 MSVC kit）
- LLVM-MinGW 工具链（如 `llvm-mingw1706_64`，MSVC 用户可省略）
- CMake ≥ 3.19

```bash
export PATH="/d/Qt/Tools/llvm-mingw1706_64/bin:$PATH"
TOOLCHAIN=/d/Qt/Tools/llvm-mingw1706_64/bin

cmake -G "MinGW Makefiles" \
  -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/d/Qt/6.11.1/llvm-mingw_64" \
  -DCMAKE_C_COMPILER="$TOOLCHAIN/x86_64-w64-mingw32-gcc.exe" \
  -DCMAKE_CXX_COMPILER="$TOOLCHAIN/x86_64-w64-mingw32-g++.exe" \
  -DCMAKE_RC_COMPILER="$TOOLCHAIN/x86_64-w64-mingw32-windres.exe"

cmake --build build --config Release
```

构建完成后 `build/` 目录即为完整可运行环境——CMake 已自动部署所有 Qt DLL 和插件，
无需手动运行 `windeployqt`。

> 也可以在 Qt Creator 中直接打开本目录（识别 `CMakeLists.txt`）
> 并选择 kit 进行构建。

**一键构建脚本：**
```bash
# 构建 + 打包 zip 分发包
./scripts/release.sh [版本号]
```

> 也可以在 Qt Creator 中直接打开本目录（识别 `CMakeLists.txt`）并选择 kit 进行构建。

## 相关项目

- [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) — 上游 CLI，本项目的后端
- [N_m3u8DL-CLI](https://github.com/nilaoda/N_m3u8DL-CLI) — 最初版本的 .NET 命令行下载器
- [N_m3u8DL-CLI-SimpleG](https://github.com/nilaoda/N_m3u8DL-CLI-SimpleG) — 带简单 GUI 的早期版本

## 目录结构

```
include/
  mainwindow.h          主窗口类声明
  options.h             参数元信息声明
  stringlistwidget.h    可重复项控件声明
src/
  main.cpp              程序入口
  mainwindow.cpp        主窗口核心
  mainwindow_ui.cpp     界面构建（输入区/顶层页签/设置页/监控区）
  mainwindow_command.cpp 命令拼装与预览
  mainwindow_task.cpp   下载执行、队列管理、日志与右键菜单
  mainwindow_clipboard.cpp 剪贴板自动填充与文件名派生
  mainwindow_settings.cpp 设置持久化、主题/置顶/重置
  mainwindow_paths.cpp  程序/ffmpeg 路径自动探测
  options.cpp           参数元信息定义（分类 + 类型 + 说明）
  stringlistwidget.cpp  可重复项编辑控件
resources/
  app_icon.png          应用图标
  app_icon.qrc          Qt 资源文件
CMakeLists.txt
doc/N_m3u8DL-RE_README.md  N_m3u8DL-RE 命令行参数说明（取自上游）
```

## 许可证

MIT License — 详见 [LICENSE](LICENSE) 文件。
