#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "test_sc_common.h"
#include "ass_util.hpp"
#include "ass_render_utils.h"

using namespace std;

constexpr auto ass_template_header = R"--([Script Info]
Title: hello
ScriptType: v4.00+
PlayResX: 1920
PlayResY: 1080

[V4+ Styles]
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding
Style: sc,微软雅黑,35,&H65FFFFFF,&H65FFFFFF,&H00000000,&H6566A514,-1,0,0,0,100,100,0,0,1,0.6,0.0,7,0,0,0,0


[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
)--";
constexpr int VIDEO_WIDTH = 1920;
constexpr int VIDEO_HEIGHT = 1080;

void read_file(vector<string> &buffer, string file_name) {
    ifstream in(file_name);

    string line;
    while (std::getline(in, line)) {
        // Line contains string of length > 0 then save it in vector
        if (line.size() > 0)
            buffer.push_back(line);
    }
}

int main() {
    constexpr bool sc_price_no_break_line = true;
    std::string tmp_user_name, tmp_content;
    std::string res;
    std::vector<std::string> sc_list;
    std::vector<std::string> username_list;
    int price = 50;
    int font_size = 35;
    int corner_radius = 17;
    int width = 450;
    int x1 = 200;
    int x2 = 200;
    int y1 = 300;
    int y2 = 300;
    int start_time = 1000;

    ASS_Library *ass_library = nullptr;
    ASS_Renderer *ass_renderer = nullptr;
    std::string sc_ass_header_str = ass_template_header;
    libass_init(&ass_library, &ass_renderer, VIDEO_WIDTH, VIDEO_HEIGHT, false);

    ass::sc_ass_render_control sc_render{ass_library};
    sc_render.create_track(const_cast<char *>(sc_ass_header_str.c_str()),
                           sc_ass_header_str.size());
    auto ass_track = sc_render.get_track();

    ass::TextProcess::Init(ass_library, ass_renderer, ass_track);

    cout << sc_ass_header_str;
    read_file(sc_list, "sc_content_list.txt");
    read_file(username_list, "sc_username_list.txt");
    int username_count = username_list.size();
    int i = 0;
    for (auto &content : sc_list) {
        std::vector<std::string> res_list;

        tmp_user_name = username_list[i++ % username_count];
        tmp_content = content;
        sc::sc_item_t sc{tmp_user_name, tmp_content, 0, price};
        ass::SuperChatMessage sc_msg{ass::SuperChatMessageUpdateType::immediate,
            std::move(sc), 0, 0, width, corner_radius, font_size, sc_price_no_break_line};

        sc_msg.getSuperChatAss(x1, y1, x2, y2, start_time, start_time + 1500, res_list);

        for (auto &item : res_list) {
            cout << item;
        }
        start_time += 1500;
        price += 50;
    }

    return 0;
}