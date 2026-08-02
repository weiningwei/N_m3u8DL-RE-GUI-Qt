# 源码目录

## 文件说明

| 文件 | 功能 |
|------|------|
| `main.cpp` | 程序入口：QApplication 初始化、便携配置、图标设置 |
| `mainwindow.cpp` | 主窗口核心：构造函数、关闭事件、退出确认、折叠面板管理 |
| `mainwindow_ui.cpp` | 界面构建：菜单栏、工具栏、输入区、控制区、参数标签页、路径/预设/命令区、监控区（下载列表 + 日志 QSplitter） |
| `mainwindow_command.cpp` | 命令拼装：读取 Entry 列表生成完整命令行、命令预览、打开输出目录 |
| `mainwindow_task.cpp` | 下载管理：任务创建、队列调度、进程启动、日志追加、预设保存/加载、右键菜单、独立终端模式 |
| `mainwindow_clipboard.cpp` | 剪贴板监听：自动填充链接、派生保存文件名 |
| `mainwindow_settings.cpp` | 设置持久化：保存/加载/恢复默认、主题切换、窗口置顶 |
| `mainwindow_paths.cpp` | 路径探测：N_m3u8DL-RE / ffmpeg 自动查找、路径校验 |
| `options.cpp` | 参数元信息：7 个分类标签页的 CLI 参数定义（类型、默认值、说明） |
| `stringlistwidget.cpp` | 可重复项控件：`-H` / `--key` / `--mux-import` 等多项值编辑 |

## 对应头文件

| 头文件 | 内容 |
|--------|------|
| `include/mainwindow.h` | MainWindow 类声明、TaskMeta / PendingTask / Entry 数据结构、槽函数 |
| `include/options.h` | Opt 参数元信息结构、Category 分类结构、buildCategories() 声明 |
| `include/stringlistwidget.h` | StringListWidget 可重复项控件声明 |
