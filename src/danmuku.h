#ifndef BILIBILI_DANMUKU_DANMUKU_H
#define BILIBILI_DANMUKU_DANMUKU_H

#include <iostream>
#include <string>
#include <vector>

#include "scn/scn.h"
#include "thirdparty/pugixml/pugixml.hpp"
//#include "thirdparty/utf8/utf8.h"
#include "thirdparty/simdutf/simdutf.h"

#include "config.h"

namespace danmuku {

//inline size_t get_utf8_len(const std::string &s) {
//    return utf8::distance(s.begin(), s.end());
//}

inline size_t get_utf8_len(const char *s) {
    return simdutf::count_utf8(s, strlen(s));
}

enum class danmu_type {
    MOVE = 1,
    BOTTOM = 4,
    TOP = 5,
};

typedef struct danmuku_info_ {
    std::string chat_server_;
    std::string chat_id_;

    danmuku_info_(std::string chat_server, std::string chat_id)
        : chat_server_(chat_server), chat_id_(chat_id){};
    danmuku_info_(){};

} danmuku_info_t;

typedef struct danmuku_item_ {
    const char *context_;
    size_t length_; // UTF8 length
    float start_time_;
    int danmuku_type_;
    int font_size_;
    int font_color_;

    //uint64_t create_time_;
    //int pool_;
    //int uid_;
    //int history_id_;

    danmuku_item_(){};

    inline void fast_sscanf(const char *token) {
        // sscanf version: sscanf("%f,%d,%d,%d") for:
        //     start_time_, danmuku_type_, font_size_, font_color_
        // and trim the space.
        char *resstr = const_cast<char *>(token);
        start_time_ = atof(token);
        if ((resstr = const_cast<char *>(strchr(resstr, ','))) != nullptr) {
            danmuku_type_ = atoi(++resstr);
            if ((resstr = const_cast<char *>(strchr(resstr, ','))) != nullptr) {
                font_size_ = atoi(++resstr);
                if ((resstr = const_cast<char *>(strchr(resstr, ','))) != nullptr) {
                    font_color_ = atoi(++resstr);
                }
            }
        }
    }

    danmuku_item_(const char *context, const char *p) : context_(context) {
        length_ = get_utf8_len(context);
        fast_sscanf(p);
        //sscanf(trim_space, "", &start_time_, &danmuku_type_)
        //        auto _ = scn::scan(trim_space,
        //                           "{},{},{},{},"
        //                           "{},{},{},{}",
        //                           start_time_, danmuku_type_, font_size_, font_color_,
        //                           create_time_, pool_, uid_, history_id_);
    };
} danmuku_item_t;

typedef struct ass_dialogue_ {
    const char *context_;
    size_t length_;
    float start_time_;
    int danmuku_type_;
    int font_size_;
    int font_color_;

    int dialogue_line_index_; // calculate the y-axis coordinates
    bool is_valid_;
} ass_dialogue_t;

int parse_danmuku_xml(pugi::xml_document &doc, pugi::xml_parse_result &parse_result,
                      std::string file_path,
                      std::vector<danmuku_item_t> &danmuku_all_list,
                      danmuku_info_t &danmuku_info);

int process_danmuku_list(const std::vector<danmuku_item_t> &danmuku_all_list,
                         std::vector<danmuku_item_t> &danmuku_move_list,
                         std::vector<danmuku_item_t> &danmuku_pos_list);

int process_danmuku_dialogue_pos(std::vector<danmuku_item_t> &danmuku_list,
                                 const config::ass_config_t &config,
                                 std::vector<ass_dialogue_t> &ass_result_list);

int process_danmuku_dialogue_move(std::vector<danmuku_item_t> &danmuku_list,
                                  const config::ass_config_t &config,
                                  std::vector<ass_dialogue_t> &ass_result_list);

int danmuku_main_process(std::string xml_file, config::ass_config_t config);

} // namespace danmuku

#endif // BILIBILI_DANMUKU_DANMUKU_H
