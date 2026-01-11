constexpr auto config_template_json =
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
    "#danmaku_pos_time": "固定弹幕的停留时间(以秒计)，设置为-1表示忽略固定弹幕",

    "sc_enable": true,
    "#sc_enable": "是否转换 Super Chat，false为不转换，true为转换",

    "sc_font_size": 35,
    "#sc_font_size": "Super Chat字号大小",

    "sc_alpha": 1.0,
    "#sc_font_alpha": "Super Chat消息的不透明度,取值为0~1.0,为0时完全透明",

    "sc_show_range": 0.7,
    "#sc_show_range": "Super Chat在屏幕上的显示范围，取值为0~1.0，为1时全屏显示",

    "sc_max_width": 450,
    "#sc_max_width": "Super Chat消息占用的最大宽度（以px计）",

    "sc_margin_x": 10,
    "#sc_margin_x": "Super Chat消息在水平方向上与屏幕边缘的边距大小（以px计）",

    "sc_y_mirror": false,
    "#sc_y_mirror": [ "Super Chat垂直方向镜像显示",
        "设置为false优先从屏幕下方显示Super Chat",
        "设置为true优先从屏幕上方显示Super Chat"
    ],

    "sc_price_no_break": true,
    "#sc_price_no_break": [
        "设置为false,总是在新的一行显示SC金额",
        "设置为true,如果可以，在用户名同一行的右端部分显示SC金额"
    ]
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
        },
        "sc_enable": {
            "type": "boolean"
        },
        "sc_font_size": {
            "type": "integer",
            "minimum": 1
        },
        "sc_alpha": {
            "type": "number",
            "minimum": 0,
            "maximum": 1
        },
        "sc_show_range": {
            "type": "number",
            "minimum": 0,
            "maximum": 1
        },
        "sc_max_width": {
            "type": "integer",
            "minimum": 10
        },
        "sc_margin_x": {
            "type": "integer",
            "minimum": 0
        },
        "sc_y_mirror": {
            "type": "boolean"
        },
        "sc_price_no_break": {
            "type": "boolean"
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