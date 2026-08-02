#include "options.h"

QVector<Category> buildCategories()
{
    QVector<Category> cats;

    // ------------------------------------------------------------------
    // 基本设置
    // ------------------------------------------------------------------
    {
        Category c;
        c.name = "基本设置";
        c.opts = {
            {Opt::PathDir, "--tmp-dir", "临时文件目录", "设置临时文件存储目录", "", {}, 0, false},
            {Opt::PathDir, "--save-dir", "输出目录", "设置输出目录", "", {}, 0, false},
            {Opt::String, "--save-name", "保存文件名", "设置保存文件名", "", {}, 0, false},
            {Opt::String, "--save-pattern", "命名模板",
             "命名模板, 变量: <SaveName> <Id> <Codecs> <Language> <Resolution> "
             "<Bandwidth> <MediaType> <Channels> <FrameRate> <VideoRange> <GroupId> <Ext>",
             "", {}, 0, false},
            {Opt::Int, "--thread-count", "下载线程数",
             "设置下载线程数 (0 = 自动, 使用CPU线程数)", "本机CPU线程数", {}, 0, false},
            {Opt::Int, "--download-retry-count", "分片重试次数",
             "每个分片下载异常时的重试次数", "3", {}, 3, false},
            {Opt::Int, "--http-request-timeout", "HTTP超时(秒)",
             "HTTP请求的超时时间(秒)", "100", {}, 100, false},
            {Opt::String, "--task-start-at", "开始时间",
             "在此时间之前不会开始执行任务, 格式 yyyyMMddHHmmss", "", {}, 0, false},
        };
        cats.append(c);
    }

    // ------------------------------------------------------------------
    // 网络
    // ------------------------------------------------------------------
    {
        Category c;
        c.name = "网络";
        c.opts = {
            {Opt::List, "-H", "请求头 (可多个)",
             "为HTTP请求设置特定的请求头, 例如: Cookie: mycookie / User-Agent: iOS",
             "", {}, 0, false},
            {Opt::String, "--base-url", "BaseURL", "设置BaseURL", "", {}, 0, false},
            {Opt::Bool, "--use-system-proxy", "使用系统代理", "使用系统默认代理", "True", {}, 0, true},
            {Opt::String, "--custom-proxy", "自定义代理",
             "设置请求代理, 如 http://127.0.0.1:8888", "", {}, 0, false},
            {Opt::String, "-R", "限速",
             "设置限速, 单位支持 Mbps 或 Kbps, 如 15M 100K", "", {}, 0, false},
            {Opt::Bool, "--append-url-params", "附加URL参数",
             "将输入Url的Params添加至分片, 对某些网站很有用 (如 kakao.com)", "False", {}, 0, false},
            {Opt::String, "--urlprocessor-args", "URL Processor参数",
             "此字符串将直接传递给URL Processor", "", {}, 0, false},
        };
        cats.append(c);
    }

    // ------------------------------------------------------------------
    // 流选择
    // ------------------------------------------------------------------
    {
        Category c;
        c.name = "流选择";
        c.opts = {
            {Opt::Bool, "--auto-select", "自动选择最佳轨道", "自动选择所有类型的最佳轨道", "False", {}, 0, false},
            {Opt::String, "-sv", "选择视频流",
             "通过正则表达式选择视频流, 如 res=\"3840*\":codecs=hvc1:for=best", "", {}, 0, false},
            {Opt::String, "-sa", "选择音频流",
             "通过正则表达式选择音频流, 如 lang=en:for=best", "", {}, 0, false},
            {Opt::String, "-ss", "选择字幕流",
             "通过正则表达式选择字幕流, 如 name=\"中文\":for=all", "", {}, 0, false},
            {Opt::String, "-dv", "去除视频流", "通过正则表达式去除符合要求的视频流", "", {}, 0, false},
            {Opt::String, "-da", "去除音频流", "通过正则表达式去除符合要求的音频流", "", {}, 0, false},
            {Opt::String, "-ds", "去除字幕流", "通过正则表达式去除符合要求的字幕流", "", {}, 0, false},
            {Opt::String, "--ad-keyword", "广告分片关键字",
             "设置广告分片的URL关键字(正则表达式)", "", {}, 0, false},
            {Opt::Bool, "--sub-only", "仅下载字幕", "只选取字幕轨道", "False", {}, 0, false},
            {Opt::Combo, "--sub-format", "字幕输出类型", "字幕输出类型", "SRT",
             {"SRT", "VTT"}, 0, false},
            {Opt::Bool, "--auto-subtitle-fix", "自动修正字幕", "自动修正字幕", "True", {}, 0, true},
        };
        cats.append(c);
    }

    // ------------------------------------------------------------------
    // 解密
    // ------------------------------------------------------------------
    {
        Category c;
        c.name = "解密";
        c.opts = {
            {Opt::List, "--key", "解密密钥 (可多个)",
             "设置解密密钥: --key KID1:KEY1 --key KID2:KEY2 ; KEY相同时直接 --key KEY", "", {}, 0, false},
            {Opt::PathFile, "--key-text-file", "密钥文件",
             "设置密钥文件, 程序按KID搜寻KEY以解密 (不建议特大文件)", "", {}, 0, false},
            {Opt::Combo, "--decryption-engine", "解密引擎",
             "设置解密时使用的第三方程序", "MP4DECRYPT",
             {"FFMPEG", "MP4DECRYPT", "SHAKA_PACKAGER"}, 0, false},
            {Opt::PathFile, "--decryption-binary-path", "解密程序路径",
             "MP4解密所用工具的全路径, 如 C:\\Tools\\mp4decrypt.exe", "", {}, 0, false},
            {Opt::Bool, "--mp4-real-time-decryption", "MP4实时解密", "实时解密MP4分片", "False", {}, 0, false},
            {Opt::Combo, "--custom-hls-method", "HLS加密方式",
             "指定HLS加密方式", "",
             {"", "AES_128", "AES_128_ECB", "CENC", "CHACHA20", "NONE", "SAMPLE_AES", "SAMPLE_AES_CTR", "UNKNOWN"}, 0, false},
            {Opt::String, "--custom-hls-key", "HLS解密KEY",
             "指定HLS解密KEY. 可以是文件, HEX或Base64", "", {}, 0, false},
            {Opt::String, "--custom-hls-iv", "HLS解密IV",
             "指定HLS解密IV. 可以是文件, HEX或Base64", "", {}, 0, false},
        };
        cats.append(c);
    }

    // ------------------------------------------------------------------
    // 混流
    // ------------------------------------------------------------------
    {
        Category c;
        c.name = "混流";
        c.opts = {
            {Opt::String, "-M", "完成后混流",
             "所有工作完成时尝试混流分离的音视频. 格式如 format=mp4 或 "
             "format=mkv:muxer=mkvmerge:bin_path=\"C:\\...\\mkvmerge.exe\"", "", {}, 0, false},
            {Opt::List, "--mux-import", "引入外部媒体 (可多个)",
             "混流时引入外部媒体文件, 如 path=zh-Hans.srt:lang=chi:name=\"中文 (简体)\"", "", {}, 0, false},
            {Opt::Bool, "--binary-merge", "二进制合并", "二进制合并", "False", {}, 0, false},
            {Opt::Bool, "--use-ffmpeg-concat-demuxer", "使用concat分离器",
             "使用 ffmpeg 合并时, 使用 concat 分离器而非 concat 协议", "False", {}, 0, false},
            {Opt::Bool, "--no-date-info", "不写入日期信息", "混流时不写入日期信息", "False", {}, 0, false},
        };
        cats.append(c);
    }

    // ------------------------------------------------------------------
    // 直播
    // ------------------------------------------------------------------
    {
        Category c;
        c.name = "直播";
        c.opts = {
            {Opt::Bool, "--live-perform-as-vod", "点播方式下载直播", "以点播方式下载直播流", "False", {}, 0, false},
            {Opt::Bool, "--live-real-time-merge", "实时合并", "录制直播时实时合并", "False", {}, 0, false},
            {Opt::Bool, "--live-keep-segments", "保留分片",
             "录制直播并开启实时合并时依然保留分片", "True", {}, 0, true},
            {Opt::Bool, "--live-pipe-mux", "管道实时混流",
             "录制直播并开启实时合并时通过管道+ffmpeg实时混流到TS文件", "False", {}, 0, false},
            {Opt::Bool, "--live-fix-vtt-by-audio", "按音频修正VTT",
             "通过读取音频文件的起始时间修正VTT字幕", "False", {}, 0, false},
            {Opt::String, "--live-record-limit", "录制时长限制",
             "录制直播时的录制时长限制, 格式 HH:mm:ss", "", {}, 0, false},
            {Opt::Int, "--live-wait-time", "列表刷新间隔(秒)",
             "手动设置直播列表刷新间隔", "", {}, 0, false},
            {Opt::Int, "--live-take-count", "首次分片数量",
             "手动设置录制直播时首次获取分片的数量", "16", {}, 16, false},
        };
        cats.append(c);
    }

    // ------------------------------------------------------------------
    // 高级
    // ------------------------------------------------------------------
    {
        Category c;
        c.name = "高级";
        c.opts = {
            {Opt::Bool, "-mt", "并发下载", "并发下载已选择的音频、视频和字幕", "False", {}, 0, false},
            {Opt::Bool, "--skip-merge", "跳过合并", "跳过合并分片", "False", {}, 0, false},
            {Opt::Bool, "--skip-download", "跳过下载", "跳过下载", "False", {}, 0, false},
            {Opt::Bool, "--check-segments-count", "校验分片数量",
             "检测实际下载的分片数量和预期数量是否匹配", "True", {}, 0, true},
            {Opt::Bool, "--del-after-done", "完成后删除临时文件", "完成后删除临时文件", "True", {}, 0, true},
            {Opt::Bool, "--write-meta-json", "输出信息JSON",
             "解析后的信息是否输出json文件", "True", {}, 0, true},
            {Opt::Bool, "--no-log", "关闭日志文件", "关闭日志文件输出", "False", {}, 0, false},
            {Opt::PathFile, "--log-file-path", "日志文件路径", "设置日志文件路径, 例如 C:\\Logs\\log.txt", "", {}, 0, false},
            {Opt::Combo, "--log-level", "日志级别", "设置日志级别", "INFO",
             {"DEBUG", "ERROR", "INFO", "OFF", "WARN"}, 0, false},
            {Opt::Combo, "--ui-language", "界面语言", "设置UI语言", "",
             {"", "en-US", "zh-CN", "zh-TW"}, 0, false},
            {Opt::Bool, "--disable-update-check", "禁用更新检测", "禁用版本更新检测", "False", {}, 0, false},
            {Opt::Bool, "--force-ansi-console", "强制ANSI终端", "强制认定终端为支持ANSI且可交互的终端", "False", {}, 0, false},
            {Opt::Bool, "--no-ansi-color", "去除ANSI颜色", "去除ANSI颜色", "False", {}, 0, false},
            {Opt::String, "--custom-range", "仅下载部分分片",
             "仅下载部分分片, 如 0-10 / 10- / -99 / 05:00-20:00", "", {}, 0, false},
            {Opt::Bool, "--allow-hls-multi-ext-map", "允许多个EXT-X-MAP",
             "允许HLS中的多个#EXT-X-MAP(实验性)", "False", {}, 0, false},
        };
        cats.append(c);
    }

    return cats;
}
