# N_m3u8DL-RE-GUI-Qt

基于 **Qt 6** 的 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 图形界面。
将 `doc/N_m3u8DL-RE_README.md` 中记录的命令行参数以分类标签页的形式呈现，并负责拼装命令行、调用
`N_m3u8DL-RE` 可执行文件、实时显示输出日志。

> 本项目是 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 的 **Qt 图形前端**，
> 自身不实现下载逻辑，所有实际的下载 / 解密 / 混流能力均来自该上游命令行工具。
> 参数参考文档 `doc/N_m3u8DL-RE_README.md` 直接取自上游仓库。

## 功能亮点

- 🗂️ **分类参数标签页**：基本设置 / 网络 / 流选择 / 解密 / 混流 / 直播 / 高级，参数一目了然。
- ⚡ **实时命令预览**：边改边生成完整命令行，一键复制，所见即所得。
- 📥 **多任务并发下载**：可同时开始多个下载，运行区实时显示任务数量，一键停止全部。
- 📋 **下载列表**：记录每次启动的下载任务，显示编号 / 名称 / 状态（运行中 · 已完成 · 异常退出），点击即可筛选该任务日志。
- 📜 **实时日志 + 按任务筛选**：stdout/stderr 实时滚动，每条日志带任务标签，按任务快速定位问题。
- 📎 **剪贴板自动填充**：复制一个链接即可自动填入，并根据链接自动派生“保存文件名”；双击文件名框可重新派生。
- ⌨️ **回车即下载**：在链接框或保存文件名框按回车直接开始下载。
- 🔍 **程序 / ffmpeg 自动探测**：自动在「程序目录 / third_party / PATH」中查找，命中时显示绿色边框提示。
- 🎨 **外观与置顶**：主题支持跟随系统 / 浅色 / 暗色；可一键窗口置顶。
- 💾 **设置记忆与恢复**：自动记住路径与参数配置，也可一键恢复默认。

## 功能（详细说明）

- 按文档把参数分为：**基本设置 / 网络 / 流选择 / 解密 / 混流 / 直播 / 高级** 七个分组。
- 支持可重复项（请求头 `-H`、解密密钥 `--key`、外部媒体 `--mux-import`）。
- 实时**命令预览**，可一键复制。
- **多任务下载**：每次点击「开始下载」都会启动一个独立进程，可同时下载多个任务；运行区显示
  “正在下载任务数量”，「停止」会终止全部进行中的任务。
- **下载列表**：`build` 运行区下方的列表记录每一次启动的下载任务，展示 `#编号 名称 — 状态`，
  运行中 / 已完成 / 异常退出以不同颜色区分；点击列表项即按该任务筛选下方日志，点击「全部」恢复显示所有日志。
- 通过 `QProcess` 调用 `N_m3u8DL-RE`，实时显示 stdout/stderr 日志，支持**开始 / 停止**；
  每条日志按任务打上 `[#编号 名称]` 前缀以便区分。
- **剪贴板自动填充**：程序启动及剪贴板出现链接（http(s)/magnet/rtsp/rtmp/本地路径）时自动填入链接框；
  同时根据链接自动派生「保存文件名」；手动修改文件名后不会被覆盖，双击文件名框可重新从链接派生。
- **回车即下载**：在「链接/文件」框或「保存文件名」框按回车，直接开始下载。
- 自动探测 `N_m3u8DL-RE.exe` 与 `ffmpeg.exe`，可手动指定。探测顺序为：
  ① GUI 程序所在目录 → ② 其下的 `third_party/` → ③ 环境变量 `PATH`。
- 路径框被自动填充时会显示**绿色边框**并提示“已自动检测到”，手动修改后提示消失。
- `N_m3u8DL-RE` 依赖 **ffmpeg** 进行合并/解密等处理：顶层提供「ffmpeg 路径」选择器，
  留空时由程序自动查找；非空则对应 `--ffmpeg-binary-path`。
- 使用 `QSettings` 记住程序路径、ffmpeg 路径、输入及所有参数配置；「设置 → 恢复默认配置」可清空并重置。
- **外观**：「设置 → 主题」支持跟随系统 / 浅色 / 暗色（需 Qt 6.5+）。
- **窗口置顶**：「设置 → 窗口置顶」可将窗口固定在最上层。
- 主题与置顶状态均会被记住。

## 快速开始

1. 准备 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 的可执行文件（`N_m3u8DL-RE.exe`），
   以及可选的 `ffmpeg.exe`（用于合并 / 解密）。
2. 将 `N_m3u8DL-RE.exe` 放到 GUI 程序所在目录或 `third_party/` 下（或确保其位于 `PATH`），
   `ffmpeg.exe` 同理；程序启动后会**自动探测**并显示绿色提示，也可在「ffmpeg路径」手动指定。
3. 复制一个 m3u8 / dash 链接，程序会自动填入「链接/文件」并派生「保存文件名」。
4. 按需在各标签页调整参数，命令预览会实时更新；确认无误后点击「开始下载」
   （或在链接框 / 文件名框按回车）。
5. 在「下载列表」中查看每个任务的状态，点击列表项即可聚焦该任务的日志。

## 构建

环境要求：

- Qt 6.11.1（本仓库示例使用的 kit 为 `llvm-mingw_64`）
- LLVM-MinGW 工具链（如 `llvm-mingw1706_64`）
- CMake ≥ 3.19、Ninja

```bash
# 在项目根目录
export PATH="/d/Qt/Tools/llvm-mingw1706_64/bin:/d/Qt/Tools/Ninja:$PATH"
TOOLCHAIN=/d/Qt/Tools/llvm-mingw1706_64/bin

cmake -GNinja \
  -S . -B build \
  -DCMAKE_PREFIX_PATH="D:/Qt/6.11.1/llvm-mingw_64" \
  -DCMAKE_C_COMPILER="$TOOLCHAIN/x86_64-w64-mingw32-gcc.exe" \
  -DCMAKE_CXX_COMPILER="$TOOLCHAIN/x86_64-w64-mingw32-g++.exe" \
  -DCMAKE_RC_COMPILER="$TOOLCHAIN/x86_64-w64-mingw32-windres.exe"

cmake --build build
```

构建完成后，用 Qt 自带的 `windeployqt` 把运行所需的 Qt 动态库复制到 `build/` 目录下：
即用 `windeployqt build/N_m3u8DL_RE_GUI_Qt.exe`。

> 也可以在 Qt Creator 中直接打开本目录（识别 `CMakeLists.txt`）并选择
> `llvm-mingw_64` kit 进行构建。

## 使用

1. 在「程序路径」中指定 `N_m3u8DL-RE.exe`（若已在 `PATH` 中会自动探测）。
2. 在「链接/文件」中输入 m3u8 / dash 链接或本地清单文件（也可直接粘贴，会自动填入）。
3. 在各标签页按需设置参数。
4. 点击「生成命令」查看拼装结果，确认无误后点击「开始下载」。
5. 日志区实时显示程序输出，可随时「停止」；「下载列表」记录任务并可按任务筛选日志。

## 相关项目

- [N_m3u8DL-CLI](https://github.com/nilaoda/N_m3u8DL-CLI) — 最初的命令行下载器，基于 .NET
- [N_m3u8DL-CLI-SimpleG](https://github.com/nilaoda/N_m3u8DL-CLI-SimpleG) — 带简单 GUI 的版本

## 目录结构

```
include/
  mainwindow.h          主窗口类声明（接口）
  options.h             参数元信息声明
  stringlistwidget.h    可重复项控件声明
src/
  main.cpp              程序入口
  mainwindow.cpp        主窗口核心：构造函数、关闭/事件过滤
  mainwindow_ui.cpp     界面构建（工具栏/输入区/参数页/运行区）
  mainwindow_command.cpp 命令拼装与预览
  mainwindow_task.cpp   下载执行、进程输出、日志、下载列表
  mainwindow_clipboard.cpp 剪贴板自动填充与文件名派生
  mainwindow_settings.cpp 设置持久化、主题/置顶/重置等动作
  mainwindow_paths.cpp  程序/ffmpeg 路径自动探测与解析
  options.cpp           参数元信息实现（分类 + 类型 + 说明）
  stringlistwidget.cpp  可重复项编辑控件（-H / --key / --mux-import）
CMakeLists.txt
doc/N_m3u8DL-RE_README.md  N_m3u8DL-RE 命令行参数说明（取自上游仓库，数据来源）
```
