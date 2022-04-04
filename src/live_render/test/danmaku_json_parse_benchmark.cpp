#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "thirdparty/rapidjson/document.h"
#include "thirdparty/re2/re2/re2.h"

using namespace std;

enum raw_dm_type {
    TEXT = 0,
    STICKER = 1,
};

void read_file(vector<string> &buffer, string file_name) {
    ifstream in(file_name);

    string line;
    while (std::getline(in, line)) {
        // Line contains string of length > 0 then save it in vector
        if (line.size() > 0)
            buffer.push_back(line);
    }
}

class ParseHelper {
  public:
    ParseHelper(){};
    RE2 *content_re_;
    RE2 *danmaku_type_re_;
    RE2 *danmaku_color_re_;
    RE2 *danmaku_info_re_;
};

struct DanmakuItem {
  public:
    int color_;
    int danmaku_origin_type_;
    int danmaku_player_type_;
    uint64_t timestamp_;
    string content_;

    DanmakuItem(int color, int danmaku_origin_type, int danmaku_player_type,
                uint64_t timestamp, string &content)
        : color_(color), danmaku_origin_type_(danmaku_origin_type),
          danmaku_player_type_(danmaku_player_type), timestamp_(timestamp),
          content_(content) {
    }
};

bool operator==(DanmakuItem const &l, DanmakuItem const &r) {
    return l.color_ == r.color_ &&
        l.danmaku_player_type_ == r.danmaku_player_type_ &&
        l.danmaku_origin_type_ == r.danmaku_origin_type_ &&
        l.timestamp_ == r.timestamp_ &&
        l.content_ == r.content_;
}

bool test_regex_parse(ParseHelper &parse_helper, vector<string> &json_line) {
    cout << "[regex]: Start" << endl;

    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    string content;

    // does not support lookahead assertion
    const auto content_re_str = R"("content\\":\\"(.*?)\\",\\")";
    parse_helper.content_re_ = new RE2(content_re_str);
    assert(parse_helper.content_re_->ok());

    const auto danmaku_type_re_str = R"(dm_type\\":(\d*))";
    parse_helper.danmaku_type_re_ = new RE2(danmaku_type_re_str);
    assert(parse_helper.danmaku_type_re_->ok());

    const auto damaku_info_re_str = R"("info":[[][[].*?,(\d),.*?,.*?,(\d*))";
    // [_, danmaku_player_type, font_size, _, timestamp]
    parse_helper.danmaku_info_re_ = new RE2(damaku_info_re_str);
    assert(parse_helper.danmaku_info_re_->ok());

    const auto danmaku_color_re_str = R"(\\"color\\":(\d*))";
    parse_helper.danmaku_color_re_ = new RE2(danmaku_color_re_str);
    assert(parse_helper.danmaku_color_re_->ok());

    for (auto &item : json_line) {
        bool res = true;
        res |= RE2::PartialMatch(item, *parse_helper.content_re_, &content);
        res |=
            RE2::PartialMatch(item, *parse_helper.danmaku_type_re_, &danmaku_origin_type);
        res |= RE2::PartialMatch(item, *parse_helper.danmaku_info_re_,
                                 &danmaku_player_type, &timestamp);
        res |= RE2::PartialMatch(item, *parse_helper.danmaku_color_re_, &color);

        if (!res) {
            cout << "[regex]: can not parse: " << item << endl;
            return false;
        }
    }

    cout << "[regex]: PASS" << endl;
    return true;
}

void test_regex_benchmark(ParseHelper &parse_helper, vector<string> &json_line,
                          vector<DanmakuItem> &danmaku_list) {

    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    string content;

    auto job_start_time = std::chrono::high_resolution_clock::now();
    for (auto &item : json_line) {
        RE2::PartialMatch(item, *parse_helper.content_re_, &content);
        RE2::PartialMatch(item, *parse_helper.danmaku_type_re_, &danmaku_origin_type);
        RE2::PartialMatch(item, *parse_helper.danmaku_info_re_, &danmaku_player_type,
                          &timestamp);
        RE2::PartialMatch(item, *parse_helper.danmaku_color_re_, &color);

        danmaku_list.emplace_back(color, danmaku_origin_type, danmaku_player_type,
                                  timestamp, content);
    }
    auto job_end_time = std::chrono::high_resolution_clock::now();
    double cost_time_ms =
        std::chrono::duration<double, std::milli>(job_end_time - job_start_time).count();

    cout << "[regex] cost: " << cost_time_ms << "ms \n";
}

bool test_json_parse(vector<string> &json_line, vector<DanmakuItem> &danmaku_list) {
    using namespace rapidjson;

    Document doc;
    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    string content;

    auto job_start_time = std::chrono::high_resolution_clock::now();
    for (auto &item : json_line) {
        doc.Parse(item.c_str());

        auto &info = doc["info"];

        danmaku_player_type = info[0][1].GetInt();
        color = info[0][3].GetInt();
        timestamp = info[0][4].GetInt64();
        danmaku_origin_type = info[0][12].GetInt();
        content = string(info[1].GetString());

        danmaku_list.emplace_back(color, danmaku_origin_type, danmaku_player_type,
                                  timestamp, content);
    }

    auto job_end_time = std::chrono::high_resolution_clock::now();
    double cost_time_ms =
        std::chrono::duration<double, std::milli>(job_end_time - job_start_time).count();

    cout << "[json] cost: " << cost_time_ms << "ms \n";

    return true;
}

int main() {
    vector<string> json_line;

    read_file(json_line, "danmaku_list.txt");

    ParseHelper parse_helper;
    vector<DanmakuItem> res1, res2;

    test_json_parse(json_line, res2);

    test_regex_parse(parse_helper, json_line);
    test_regex_benchmark(parse_helper, json_line, res1);

    for (auto i = 0; i < res1.size(); i++) {
        if (!(res1[i] == res2[i])) {
            std::cout << "no" << endl;
        }
    }

    return 0;
}