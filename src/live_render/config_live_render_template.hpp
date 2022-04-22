constexpr auto config_live_render_template_json =
    R"--({
    "ffmpeg_path": "tool/",
    "#ffmpeg_path": "ffmpeg所在的路径，例如ffmpeg位于tool文件夹下",

    "output_path": "video/",
    "#output_path": "视频存放路径，例如存放在video文件夹下",

    "video_bitrate": "15M",
    "#video_bitrate": "视频流比特率，按照ffmpeg接受的格式输入",

    "audio_bitrate": "320K",
    "#audio_bitrate": "音频流比特率，按照ffmpeg接受的格式输入。如果需要输出原始音频流，请设置为copy",

    "decoder": "nvdec",
    "#decoder": [
        " 视频的硬件解码器类型，可能的值有",
        " none (不使用硬件解码器) , nvdec (nvidia gpu), qsv (intel gpu), dxav2 (仅用于windows), d3d11va (仅用于windows) ",
        " 注意，这些值并未经过广泛测试，且不建议采取其他值(如：不支持cuda)"
    ],

    "encoder": "hevc_nvenc",
    "#encoder": [
        " 视频的软/硬件编码器类型，可能的值有",
        " hevc_nvenc (nvidia gpu h265), h264_nvenc (nvidia gpu h264)",
        " h264_amf (amd gpu h264), hevc_amf (amd gpu h265), libx264 (cpu h264 软件编码), libx265 (cpu h265 软件编码)",
        " h264_qsv (intel gpu h264), hevc_qsv (intel gpu h265) 等。",
        " 或者您可以选择一个ffmpeg接受的编码器"
    ],

    "extra_encoder_info": [ ""
    ],
    "#extra_encoder_info": [
        "您希望传递给编码器的额外信息，例如您可能想要调整预设，如果您想传递的参数为 `-preset 15` 需要这样做：",
        ["-preset", "15"],
        "每个字段用空格隔开即可。如果您不想传递额外信息，保持上面的项目不变即可。"
    ],

    "segment_time": 0,
    "#segment_time" : "视频切片长度（以秒计），0表示不切片",

    "ffmpeg_thread_queue_size": 20000,
    "#ffmpeg_thread_queue_size": "拉流线程队列大小，一般不调节此项。详见FAQ",

    "render_thread_queue_size": 64,
    "#render_thread_queue_size": "渲染线程队列大小，详见FAQ",


    "post_convert": true,
    "#post_convert": "是否在录制结束后自动将格式转换为faststart形式（faststart可以加快视频加载的时间)",

    "font_family": "微软雅黑",
    "#font_family": "采用的字体集",

    "font_scale": 1.6,
    "#font_scale": "字体缩放倍数，为1.0时保持原始大小（基础字号为25）",

    "font_alpha": 0.7,
    "#font_alpha": "字体透明度,取值为0~1.0,为0时完全透明",

    "font_bold": true,
    "#font_bold": "是否设置字体加粗,true加粗,false不加粗",

    "font_outline": 1.0,
    "#font_outline": "字体描边（边框）值",

    "font_shadow": 0.0,
    "#font_shadow": "字体阴影值",

    "danmaku_show_range": 0.5,
    "#danmaku_show_range": "弹幕在屏幕上的显示范围，取值为0~1.0，为1时全屏显示",

    "danmaku_move_time": 15,
    "#danmaku_move_time": "滚动弹幕的停留时间(以秒计)",

    "danmaku_pos_time": 0,
    "#danmaku_pos_time": "固定弹幕的停留时间(以秒计)，为0时忽略固定弹幕",

    "danmaku_lead_time_compensation": -6000,
    "#danmaku_pos_time": [ "弹幕超前补偿时间(以毫秒计)", "注意将您的本机时间与北京时间同步",
        "该值必须小于等于0", "当该值的绝对值越大时，弹幕越后出现",
        "例如-7000的弹幕将比-6000的弹幕更晚出现"
    ],

    "vertical_danmaku_strategy": 2,
    "#vertical_danmaku_strategy": [ "竖版弹幕处理策略", "0不处理",
        "1直接丢弃所有竖版弹幕", "2将竖版弹幕转为横版弹幕"
    ],

    "verbose": 0,
    "#verbose": [ "控制台输出等级设定", "0为默认输出", "1屏蔽所有ffmpeg输出", "2屏蔽所有弹幕信息输出",
       "3屏蔽所有ffmpeg和弹幕信息输出", "4屏蔽所有一般统计信息", "5屏蔽所有ffmpeg和一般统计信息输出",
       "6屏蔽所有统计信息和弹幕信息输出", "7屏蔽所有ffmpeg、弹幕信息和一般统计信息输出"
    ],

    "video_width": 1920,
    "#video_width": "强制设置视频宽度，一般情况下此项将被忽略",

    "video_height": 1080,
    "#video_height": "强制设置视频高度，一般情况下此项将被忽略",

    "fps": 60,
    "#fps": "强制设置视频帧率，一般情况下此项将被忽略"

}
)--"; // \n here

constexpr auto config_live_render_template_schema =
    R"--({
    "type": "object",
    "properties": {
        "ffmpeg_path": {
            "type": "string"
        },
        "output_path": {
            "type": "string"
        },
        "video_bitrate": {
            "type": "string"
        },
        "audio_bitrate": {
            "type": "string"
        },
        "decoder": {
            "type": "string"
        },
        "encoder": {
            "type": "string"
        },
        "extra_encoder_info": {
            "type": "array",
            "items": {
                "type": "string"
            }
        },
        "ffmpeg_thread_queue_size": {
            "type": "number",
            "minimum": 1
        },
        "render_thread_queue_size": {
            "type": "number",
            "minimum": 1
        },
        "segment_time": {
            "type": "number",
            "minimum": 0
        },
        "video_width": {
            "type": "integer"
        },
        "video_height": {
            "type": "integer"
        },
        "font_family": {
            "type": "string"
        },
        "font_scale": {
            "type": "number",
            "minimum": 0
        },
        "font_alpha": {
            "type": "number",
            "minimum": 0,
            "maximum": 1
        },
        "font_bold": {
            "type": "boolean"
        },
        "font_outline": {
            "type": "number",
            "minimum": 0
        },
        "font_shadow": {
            "type": "number",
            "minimum": 0
        },
        "post_convert": {
            "type": "boolean"
        },
        "danmaku_show_range": {
            "type": "number",
            "minimum": 0,
            "maximum": 1
        },
        "danmaku_move_time": {
            "type": "integer",
            "minimum": 1
        },
        "danmaku_pos_time": {
            "type": "integer",
            "minimum": 0
        },
        "danmaku_lead_time_compensation": {
            "type": "integer",
            "maximum": 0
        },
        "vertical_danmaku_strategy": {
            "type": "integer",
            "enum": [0, 1, 2]
        },
        "verbose": {
            "type": "integer"
        }
    },
    "required": [
        "ffmpeg_path",
        "output_path",
        "video_bitrate",
        "audio_bitrate",
        "decoder",
        "encoder",
        "extra_encoder_info",
        "ffmpeg_thread_queue_size",
        "render_thread_queue_size",
        "post_convert",
        "font_family",
        "font_scale",
        "font_alpha",
        "font_bold",
        "font_outline",
        "font_shadow",
        "danmaku_show_range",
        "danmaku_move_time",
        "danmaku_pos_time",
        "danmaku_lead_time_compensation",
        "vertical_danmaku_strategy",
        "verbose"
    ]
}
)--"; // \n here
