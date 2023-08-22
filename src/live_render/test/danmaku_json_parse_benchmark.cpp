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

    // sc type
    RE2 *sc_content_re_;
    RE2 *sc_user_name_re_;
    RE2 *sc_price_re_;
    RE2 *sc_start_time_re_;
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
    return l.color_ == r.color_ && l.danmaku_player_type_ == r.danmaku_player_type_ &&
           l.danmaku_origin_type_ == r.danmaku_origin_type_ &&
           l.timestamp_ == r.timestamp_ && l.content_ == r.content_;
}

struct ScItem {
    std::string user_name_;
    std::string content_;
    uint64_t start_time_;
    int price_;

    ScItem(std::string &user_name, std::string &content, uint64_t start_time, int price)
        : user_name_(user_name), content_(content), start_time_(start_time),
          price_(price) {
    }
};

bool operator==(ScItem const &l, ScItem const &r) {
    return l.user_name_ == r.user_name_ && l.content_ == r.content_ &&
           l.start_time_ == r.start_time_ && l.price_ == r.price_;
}

bool is_type_match(const char *content, const char *type_name, int type_name_len) {
    auto offset =
        std::search(content, content + 30, type_name, type_name + type_name_len);
    bool type_match = offset != (content + 30);
    return type_match;
};

bool test_regex_parse(ParseHelper &parse_helper, vector<string> &json_line) {
    cout << "[regex]: Start" << endl;

    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    string content;

    std::string user_name, sc_content;
    int price;
    uint64_t start_time;

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

    const auto sc_content_re_str = R"(message\":\"(.*?)\",\")";
    parse_helper.sc_content_re_ = new RE2(sc_content_re_str);
    assert(parse_helper.sc_content_re_->ok());

    const auto sc_user_name_re_str = R"(uname\":\"(.*?)\",\")";
    parse_helper.sc_user_name_re_ = new RE2(sc_user_name_re_str);
    assert(parse_helper.sc_user_name_re_->ok());

    const auto sc_price_re_str = R"(price\":(\d*))";
    parse_helper.sc_price_re_ = new RE2(sc_price_re_str);
    assert(parse_helper.sc_price_re_->ok());

    const auto sc_start_time_re_str = R"(ts\":(\d*))";
    parse_helper.sc_start_time_re_ = new RE2(sc_start_time_re_str);
    assert(parse_helper.sc_start_time_re_->ok());

    for (auto &item : json_line) {
        bool res = true;

        bool is_danmaku = is_type_match(item.c_str(), "DANMU", 5);
        if (is_danmaku) {
            res |= RE2::PartialMatch(item, *parse_helper.content_re_, &content);
            res |= RE2::PartialMatch(item, *parse_helper.danmaku_type_re_,
                                     &danmaku_origin_type);
            res |= RE2::PartialMatch(item, *parse_helper.danmaku_info_re_,
                                     &danmaku_player_type, &timestamp);
            res |= RE2::PartialMatch(item, *parse_helper.danmaku_color_re_, &color);
        } else {
            res |= RE2::PartialMatch(item, *(parse_helper.sc_content_re_), &sc_content);
            res |= RE2::PartialMatch(item, *(parse_helper.sc_user_name_re_), &user_name);
            res |= RE2::PartialMatch(item, *(parse_helper.sc_price_re_), &price);
            res |=
                RE2::PartialMatch(item, *(parse_helper.sc_start_time_re_), &start_time);
        }

        if (!res) {
            std::string item_type = is_danmaku ? "danmaku" : "sc";
            cout << "[regex]: can not parse " << item_type << " :" << item << endl;
            return false;
        }
    }

    cout << "[regex]: PASS" << endl;
    return true;
}

void test_regex_benchmark(ParseHelper &parse_helper, vector<string> &json_line,
                          vector<DanmakuItem> &danmaku_list, vector<ScItem> &sc_list) {

    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    string content;

    std::string user_name, sc_content;
    int price;
    uint64_t start_time;

    auto job_start_time = std::chrono::high_resolution_clock::now();
    for (auto &item : json_line) {
        bool is_danmaku = is_type_match(item.c_str(), "DANMU", 5);
        if (is_danmaku) {
            RE2::PartialMatch(item, *parse_helper.content_re_, &content);
            RE2::PartialMatch(item, *parse_helper.danmaku_type_re_, &danmaku_origin_type);
            RE2::PartialMatch(item, *parse_helper.danmaku_info_re_, &danmaku_player_type,
                              &timestamp);
            RE2::PartialMatch(item, *parse_helper.danmaku_color_re_, &color);

            danmaku_list.emplace_back(color, danmaku_origin_type, danmaku_player_type,
                                      timestamp, content);
        } else {
            RE2::PartialMatch(item, *(parse_helper.sc_content_re_), &sc_content);
            RE2::PartialMatch(item, *(parse_helper.sc_user_name_re_), &user_name);
            RE2::PartialMatch(item, *(parse_helper.sc_price_re_), &price);
            RE2::PartialMatch(item, *(parse_helper.sc_start_time_re_), &start_time);
            sc_list.emplace_back(user_name, sc_content, start_time, price);
        }
    }
    auto job_end_time = std::chrono::high_resolution_clock::now();
    double cost_time_ms =
        std::chrono::duration<double, std::milli>(job_end_time - job_start_time).count();

    cout << "[regex] cost: " << cost_time_ms << "ms \n";
}

bool test_json_parse(vector<string> &json_line, vector<DanmakuItem> &danmaku_list,
                     vector<ScItem> &sc_list) {
    using namespace rapidjson;

    Document doc;
    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    string content;

    std::string user_name, sc_content;
    int price;
    uint64_t start_time;

    auto job_start_time = std::chrono::high_resolution_clock::now();
    for (auto &item : json_line) {
        doc.Parse(item.c_str());

        bool is_danmaku = is_type_match(item.c_str(), "DANMU", 5);
        if (is_danmaku) {
            auto &info = doc["info"];

            danmaku_player_type = info[0][1].GetInt();
            color = info[0][3].GetInt();
            timestamp = info[0][4].GetInt64();
            danmaku_origin_type = info[0][12].GetInt();
            content = string(info[1].GetString());

            danmaku_list.emplace_back(color, danmaku_origin_type, danmaku_player_type,
                                      timestamp, content);
        } else {
            // sc process
            auto &data = doc["data"];

            user_name = data["user_info"]["uname"].GetString();
            sc_content = data["message"].GetString();
            price = data["price"].GetInt();
            start_time = data["ts"].GetInt64();

            sc_list.emplace_back(user_name, sc_content, start_time, price);
        }
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
    vector<ScItem> sc_res1, sc_res2;

    test_json_parse(json_line, res2, sc_res2);

    test_regex_parse(parse_helper, json_line);
    test_regex_benchmark(parse_helper, json_line, res1, sc_res1);

    if (res1.size() != res2.size()) {
        cout << "danmaku list size not match" << endl;
    }

    if (sc_res1.size() != sc_res2.size()) {
        cout << "sc list size not match" << endl;
    }

    for (auto i = 0; i < res1.size(); i++) {
        if (!(res1[i] == res2[i])) {
            std::cout << "danmaku list no match" << endl;
        }
    }

    for (auto i = 0; i < sc_res1.size(); i++) {
        if (!(sc_res1[i] == sc_res2[i])) {
            cout << "sc list not match" << endl;
        }
    }

    cout << endl << "Done." << endl;

    return 0;
}