# N_m3u8DL-RE-GUI-Qt

基于 **Qt 6** 的 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 图形界面。
将 `doc/N_m3u8DL-RE_README.md` 中记录的命令行参数以分类标签页的形式呈现，并负责拼装命令行、调用
`N_m3u8DL-RE` 可执行文件、实时显示输出日志。

> 本项目是 [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-RE) 的 **Qt 图形前端**，
> 自身不实现下载逻辑，所有实际的下载 / 解密 / 混流能力均来自该上游命令行工具。
> 参数参考文档 `doc/N_m3u8DL-RE_README.md` 直接取自上游仓库。

## 功能

- 按文档把参数分为：**基本设置 / 网络 / 流选择 / 解密 / 混流 / 直播 / 高级** 七个分组。
- 支持可重复项（请求头 `-H`、解密密钥 `--key`、外部媒体 `--mux-import`）。
- 实时**命令预览**，可一键复制。
- 通过 `QProcess` 调用 `N_m3u8DL-RE`，实时显示 stdout/stderr 日志，支持**开始 / 停止**。
- 自动探测 `N_m3u8DL-RE.exe` 与 `ffmpeg.exe`，可手动指定。探测顺序为：
  ① GUI 程序所在目录 → ② 其下的 `third_party/` → ③ 环境变量 `PATH`。
- 路径框被自动填充时会显示**绿色边框**并提示"已自动检测到"，手动修改后提示消失。
- `N_m3u8DL-RE` 依赖 **ffmpeg** 进行合并/解密等处理：顶层提供「ffmpeg 路径」选择器，
  留空时由程序自动查找；非空则对应 `--ffmpeg-binary-path`。
- 使用 `QSettings` 记住程序路径、ffmpeg 路径、输入及所有参数配置。
- **外观**：「设置 → 主题」支持跟随系统 / 浅色 / 暗色（需 Qt 6.5+）。
- **窗口置顶**：「设置 → 窗口置顶」可将窗口固定在最上层。
- 主题与置顶状态均会被记住。

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
2. 在「链接/文件」中输入 m3u8 / dash 链接或本地清单文件。
3. 在各标签页按需设置参数。
4. 点击「生成命令」查看拼装结果，确认无误后点击「开始下载」。
5. 日志区实时显示程序输出，可随时「停止」。

## 目录结构

```
src/
  main.cpp            程序入口
  mainwindow.h/.cpp   主窗口：输入区、参数标签页、运行区与命令拼装/执行
  options.h/.cpp      从文档派生的参数元信息（分类 + 类型 + 说明）
  stringlistwidget.*  可重复项编辑控件（-H / --key / --mux-import）
CMakeLists.txt
doc/N_m3u8DL-RE_README.md  N_m3u8DL-RE 命令行参数说明（取自上游仓库，数据来源）
```
