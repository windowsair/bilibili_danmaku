﻿constexpr auto config_template_json =
R"--({
    "video_width": 1920,
    "#video_width": "视频宽度",

    "video_height": 1080,
    "#video_height": "视频高度",

    "use_custom_style": false,
    "#use_custom_style": [ "是否使用自定义的ASS样式。（谨慎使用）",
        "设置为true后，将读取相同目录下的custom_style.ass文件", "每一行弹幕对应一个样式",
        "如第一行弹幕对应Danmu1, 第二行对应Danmu2，以此类推。"
    ],

    "font_family": "微软雅黑",
    "#font_family": "采用的字体集",

    "font_scale": 1.6,
    "#font_scale": "字体缩放倍数，为1.0时保持原始大小",

    "font_alpha": 0.75,
    "#font_alpha": "字体透明度,取值为0~1.0,为0时完全透明",

    "font_bold": true,
    "#font_bold": "是否设置字体加粗,true加粗,false不加粗",

    "font_outline": 1.0,
    "#font_outline": "字体描边（边框）值",

    "font_shadow": 0.0,
    "#font_shadow": "字体阴影值",

    "danmaku_show_range": 0.45,
    "#danmaku_show_range": "弹幕在屏幕上的显示范围，取值为0~1.0，为1时全屏显示",

    "danmaku_move_time": 15,
    "#danmaku_move_time": "滚动弹幕的停留时间(以秒计)，设置为-1表示忽略滚动弹幕",

    "danmaku_pos_time": 5,
    "#danmaku_pos_time": "固定弹幕的停留时间(以秒计)，设置为-1表示忽略固定弹幕"
}
)--"; // \n here

constexpr auto config_template_schema =
R"--({
    "type": "object",
    "properties": {
        "video_width": {
            "type": "integer"
        },
        "video_height": {
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
        "danmaku_show_range": {
            "type": "number",
            "minimum": 0,
            "maximum": 1
        },
        "danmaku_move_time": {
            "type": "integer"
        },
        "danmaku_pos_time": {
            "type": "integer"
        }
    },
    "required": [
        "video_width",
        "video_height",
        "use_custom_style",
        "font_family",
        "font_scale",
        "font_alpha",
        "font_bold",
        "font_outline",
        "font_shadow",
        "danmaku_show_range",
        "danmaku_move_time",
        "danmaku_pos_time"
    ]
})--";