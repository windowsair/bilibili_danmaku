#ifndef BILIBILI_DANMAKU_DANMAKU_H
#define BILIBILI_DANMAKU_DANMAKU_H

#include <iostream>
#include <string>
#include <vector>

#include "thirdparty/pugixml/pugixml.hpp"
//#include "thirdparty/utf8/utf8.h"
#include "thirdparty/simdutf/simdutf.h"

#include "ass_config.h"

namespace danmaku {

inline size_t get_utf8_len(const char *s) {
    return simdutf::count_utf8(s, strlen(s));
}

inline size_t get_utf8_len(const std::string &s) {
    return simdutf::count_utf8(s.c_str(), s.size());
}

enum class danmu_type {
    MOVE = 1,
    BOTTOM = 4,
    TOP = 5,
};

typedef struct danmaku_info_ {
    std::string chat_server_;
    std::string chat_id_;

    danmaku_info_(std::string chat_server, std::string chat_id)
        : chat_server_(chat_server), chat_id_(chat_id){};
    danmaku_info_(){};

} danmaku_info_t;

typedef struct danmaku_item_ {
    const char *context_; // We always use this item. If raw_string_ is used,
                          // then the context should point to the data of raw_string_ .
    std::string raw_string_;
    size_t length_; // UTF8 length
    float start_time_;
    int danmaku_type_;
    int font_size_;
    int font_color_;


    //uint64_t create_time_;
    //int pool_;
    //int uid_;
    //int history_id_;

    danmaku_item_(std::string &raw_string, float start_time, int danmaku_type,
                  int font_size, int font_color)
        : raw_string_(std::move(raw_string)), start_time_(start_time),
          danmaku_type_(danmaku_type), font_size_(font_size), font_color_(font_color) {

        context_ = raw_string_.c_str(); // Be careful!
                                        // This can easily lead to memory problems.
        length_ = get_utf8_len(raw_string_);
    };

    // Only used when raw_string is set and the context needs to be read.
    // Be careful! This can easily lead to memory problems.
    void update_context() {
        context_ = raw_string_.c_str();
    }


    inline void fast_sscanf(const char *token) {
        // sscanf version: sscanf("%f,%d,%d,%d") for:
        //     start_time_, danmaku_type_, font_size_, font_color_
        // and trim the space.
        char *resstr = const_cast<char *>(token);
        start_time_ = atof(token);
        if ((resstr = const_cast<char *>(strchr(resstr, ','))) != nullptr) {
            danmaku_type_ = atoi(++resstr);
            if ((resstr = const_cast<char *>(strchr(resstr, ','))) != nullptr) {
                font_size_ = atoi(++resstr);
                if ((resstr = const_cast<char *>(strchr(resstr, ','))) != nullptr) {
                    font_color_ = atoi(++resstr);
                }
            }
        }
    }

    danmaku_item_(const char *context, const char *p) : context_(context) {
        length_ = get_utf8_len(context);
        fast_sscanf(p);
        //sscanf(trim_space, "", &start_time_, &danmaku_type_)
        //        auto _ = scn::scan(trim_space,
        //                           "{},{},{},{},"
        //                           "{},{},{},{}",
        //                           start_time_, danmaku_type_, font_size_, font_color_,
        //                           create_time_, pool_, uid_, history_id_);
    };
} danmaku_item_t;

typedef struct ass_dialogue_ {
    const char *context_;
    size_t length_;
    float start_time_;
    int danmaku_type_;
    int font_size_;
    int font_color_;

    int dialogue_line_index_; // calculate the y-axis coordinates
    bool is_valid_;
} ass_dialogue_t;

class DanmakuHandle {
  public:
    DanmakuHandle() : danmaku_line_count_(0) {
    }

    void init_danmaku_screen_dialogue(const config::ass_config_t &config);

    int parse_danmaku_xml(pugi::xml_document &doc, pugi::xml_parse_result &parse_result,
                          std::string file_path,
                          std::vector<danmaku_item_t> &danmaku_all_list,
                          danmaku_info_t &danmaku_info);
    int process_danmaku_list(const std::vector<danmaku_item_t> &danmaku_all_list,
                             std::vector<danmaku_item_t> &danmaku_move_list,
                             std::vector<danmaku_item_t> &danmaku_pos_list);
    int process_danmaku_dialogue_pos(std::vector<danmaku_item_t> &danmaku_list,
                                     const config::ass_config_t &config,
                                     std::vector<ass_dialogue_t> &ass_result_list);
    int process_danmaku_dialogue_move(std::vector<danmaku_item_t> &danmaku_list,
                                      const config::ass_config_t &config,
                                      std::vector<ass_dialogue_t> &ass_result_list);
    int danmaku_main_process(std::string xml_file, config::ass_config_t config);

  private:
    int danmaku_line_count_;
    std::vector<ass_dialogue_t> top_screen_dialogue_;
    std::vector<ass_dialogue_t> bottom_screen_dialogue_;
    std::vector<ass_dialogue_t> move_screen_dialogue_;
};

} // namespace danmaku

#endif // BILIBILI_DANMAKU_DANMAKU_H
