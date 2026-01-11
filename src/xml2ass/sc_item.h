#ifndef BILIBILI_DANMAKU_SC_ITEM_H
#define BILIBILI_DANMAKU_SC_ITEM_H

#include <string>

#include "thirdparty/simdutf/simdutf.h"

namespace sc {

inline size_t get_utf8_len(const char *s) {
    return simdutf::count_utf8(s, strlen(s));
}

inline size_t get_utf8_len(const std::string &s) {
    return simdutf::count_utf8(s.c_str(), s.size());
}

typedef struct sc_item_ {
    std::string user_name_;
    std::string content_;
    size_t user_name_len_;
    size_t content_len_;
    uint64_t start_time_; // TODO: is u32 type ok?
    int price_;

    sc_item_(std::string &user_name, std::string &content, uint64_t start_time, int price)
        : user_name_(std::move(user_name)), content_(std::move(content)),
          start_time_(start_time), price_(price) {
        user_name_len_ = get_utf8_len(user_name_);
        content_len_ = get_utf8_len(content_);
    }

    sc_item_(sc_item_ &&other) = default;
    sc_item_ &operator=(sc_item_ &&other) = default;

    sc_item_(const sc_item_ &) = delete;
    sc_item_ &operator=(const sc_item_ &) = delete;
} sc_item_t;

} // namespace sc

#endif //BILIBILI_DANMAKU_SC_ITEM_H
