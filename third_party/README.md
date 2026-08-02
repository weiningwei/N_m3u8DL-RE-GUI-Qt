# third_party

存放第三方可执行文件，GUI 启动时会自动探测。

## 需要放什么

| 文件 | 说明 | 下载地址 |
|------|------|----------|
| `N_m3u8DL-RE.exe` | 核心下载器 | https://github.com/nilaoda/N_m3u8DL-RE/releases |
| `ffmpeg.exe` | 合并 / 解密（可选） | https://ffmpeg.org/download.html |

## 探测顺序

程序启动时按以下顺序查找上述文件：
1. 程序所在目录
2. `third_party/` 目录
3. 系统 `PATH` 环境变量

找到后路径框会显示绿色边框提示。

## 注意

此目录下的可执行文件不会被 git 跟踪（.gitignore 已忽略 .exe/.dll/.zip/.7z）。
