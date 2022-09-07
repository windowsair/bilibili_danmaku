#include <filesystem>
#include <fstream>
#include <iostream>

#include "danmaku_filter.h"

#include "thirdparty/fmt/include/fmt/color.h"

inline constexpr auto user_live_render_blacklist_path = "danmaku_blacklist.txt";

void DanmakuFilter::filter_init() {
    this->init_blacklist();
}

void DanmakuFilter::init_blacklist() {
    std::filesystem::path file_path(user_live_render_blacklist_path);

    if (!std::filesystem::exists(file_path)) {
        this->is_blacklist_used_ = false;
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "文件不存在\n");
        return;
    } else {
        this->is_blacklist_used_ = true;
    }

    std::string filename(user_live_render_blacklist_path);
    std::vector<std::string> regex_lines;
    std::string line;

    std::ifstream input_file(filename);
    if (!input_file.is_open()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "无法读取弹幕黑名单文件");
        std::abort();
    }

    while (getline(input_file, line)) {
        regex_lines.push_back(line);
    }

    input_file.close();

    for (auto i = 0; i < regex_lines.size(); i++) {
        auto &item = regex_lines[i];
        if (item.empty()) {
            continue;
        }

        RE2 *p = new RE2(item);
        if (p == nullptr || !p->ok()) {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                       "弹幕黑名单第{}行无效:{}", i, item);
            std::abort();
        }

        this->blacklist_regex_.push_back(p);
    }

    fmt::print(fg(fmt::color::green_yellow), "已启用弹幕黑名单，共{}条规则\n",
               this->blacklist_regex_.size());
}

bool DanmakuFilter::danmaku_item_pre_process(danmaku::danmaku_item_t& item) {

    // add more custom rules here...
    bool ret = true;

    // check blacklist
    if (this->is_blacklist_used_) {
        for (auto re2_obj : this->blacklist_regex_) {
            if (RE2::PartialMatch(item.context_, *(re2_obj))) {
                return false;
            }
        }
    }

    return ret;
}
