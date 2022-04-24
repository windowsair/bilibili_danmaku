constexpr auto ass_header_format =
R"([Script Info]
Title: {title}
ChatServer: {chat_server}
ChatId: {chat_id}
Count: {event_count}
ScriptType: v4.00+
PlayResX: {play_res_x}
PlayResY: {play_res_y}

[V4+ Styles]
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding
Style: {name},{font_name},{font_size},&H{font_alpha}{font_color},&H{font_alpha}FFFFFF,&H00000000,&H{font_alpha}000000,{font_bold},0,0,0,100,100,0,0,1,{font_outline},{font_shadow},2,20,20,2,0

[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
)"; // \n here

constexpr auto ass_dialogue_format =
R"(Dialogue: {},{},{},{},,69,69,5,,{{\{}({:.1f},{:.1f},{:.1f},{:.1f}){}}}{}
)"; // \n here


