constexpr auto config_live_render_template_json =
    R"--({
    "version": "0.0.20",

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

    "extra_input_stream_info": "",
    "#extra_input_info": [ "额外的ffmpeg输入流选项，可用于实现水印、裁剪、缩放等高级功能",
        "详见https://github.com/windowsair/bilibili_danmaku/blob/master/doc/live_render_custom_feature.md",
        "如果您不想传递额外信息，保持上面的项目不变即可。"
    ],

    "extra_filter_info": "",
    "#extra_filter_info": [ "额外的ffmpeg filter选项，可用于实现水印、裁剪、缩放等高级功能",
        "详见https://github.com/windowsair/bilibili_danmaku/blob/master/doc/live_render_custom_feature.md",
        "如果您不想传递额外信息，保持上面的项目不变即可。"
    ],

    "segment_time": 0,
    "#segment_time" : "视频切片长度（以秒计），0表示不切片",

    "ffmpeg_thread_queue_size": 20000,
    "#ffmpeg_thread_queue_size": "拉流线程队列大小，一般不调节此项。详见FAQ",

    "render_thread_queue_size": 64,
    "#render_thread_queue_size": "渲染线程队列大小，详见FAQ",


    "post_convert": true,
    "#post_convert": "是否在录制结束后自动将格式转换为faststart形式（faststart可以加快视频加载的时间)",

    "use_custom_style": false,
    "#use_custom_style": [ "是否使用自定义的ASS样式。（谨慎使用）",
        "设置为true后，将读取相同目录下的custom_style.ass文件", "每一行弹幕对应一个样式",
        "如第一行弹幕对应Danmu1, 第二行对应Danmu2，以此类推。"
    ],

    "font_family": "微软雅黑",
    "#font_family": "采用的字体集",

    "font_scale": 1.8,
    "#font_scale": "字体缩放倍数，为1.0时保持原始大小（基础字号为25）",

    "font_alpha": 0.7,
    "#font_alpha": "字体不透明度,取值为0~1.0,为0时完全透明",

    "font_alpha_fix": false,
    "#font_alpha_fix": [ "为false时，采用默认的alpha混合策略（速度优先）。弹幕可能变暗，尤其是当不透明度小于0.6时",
        "为true时，采用符合自觉的alpha混合策略（质量优先），但是会降低渲染速度",
        "根据对渲染效果和渲染速度的要求选择合适的项目。"
    ],

    "font_bold": true,
    "#font_bold": "是否设置字体加粗,true加粗,false不加粗",

    "font_outline": 0.6,
    "#font_outline": "字体描边（边框）值",

    "font_shadow": 0.0,
    "#font_shadow": "字体阴影值",

    "danmaku_show_range": 0.5,
    "#danmaku_show_range": "弹幕在屏幕上的显示范围，取值为0~1.0，为1时全屏显示",

    "danmaku_move_time": 12,
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

    "bilibili_proxy_address": "",
    "#bilibili_proxy_address": "bilibili的代理地址，仅用于获取直播流地址。如果您不清楚这是做什么的，不要修改此项。例子： https://api.live.bilibili.com/",


    "adjust_input_video_width": 0,
    "#adjust_input_video_width": [ "重新调整原始直播源视频的宽度，如果不需要调整，设置为0即可",
        "在一般情况下，live_render会使用FFmpeg中的scale对原始直播源视频的尺寸进行调整。",
        "特别地，如果原始直播视频源是一个竖版视频，而此处设置的调整后的视频为横版视频，",
        "live_render将尝试先在原始直播源视频上添加黑边，然后再调整为相应的视频尺寸。",
        "这一特性可以用于将720P的竖版直播视频转换为1080P的横版视频",
        "随意调整此项，可能会影响性能"
    ],

    "adjust_input_video_height": 0,
    "#adjust_input_video_height": [ "重新调整原始直播源视频的高度，如果不需要调整，设置为0即可",
        "随意调整此项，可能会影响性能"
    ],

    "adjust_input_video_fps": 0,
    "#adjust_input_video_fps": [ "重新调整原始直播源视频的帧率，如果不需要调整，设置为0即可",
        "如果设置了此项，会使用FFmpeg中的fps filter对原始直播源视频的帧率进行预调整",
        "随意调整此项，可能会影响性能"
    ],

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
        "extra_input_stream_info": {
            "type": "string"
        },
        "extra_filter_info": {
            "type": "string"
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
        "adjust_input_video_width": {
            "type": "integer"
        },
        "adjust_input_video_height": {
            "type": "integer"
        },
        "adjust_input_video_fps": {
            "type": "integer"
        },
        "use_custom_style": {
            "type": "boolean"
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
        "font_alpha_fix": {
            "type": "boolean"
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
        },
        "bilibili_proxy_address": {
            "type": "string"
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
        "use_custom_style",
        "font_family",
        "font_scale",
        "font_alpha",
        "font_alpha_fix",
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
