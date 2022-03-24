#ifndef BILIBILI_DANMUKU_CONFIG_H
#define BILIBILI_DANMUKU_CONFIG_H

#include <string>

namespace config {
typedef struct ass_config_ {
    // user config

    // video parameter config
    int video_width_;
    int video_height_;

    // font config
    std::string font_family_;
    int font_color_;
    int font_size_;
    float font_scale_;
    float font_alpha_;
    bool font_bold_;

    // danmuku config
    float danmuku_show_range_;
    int danmuku_move_time_;
    int danmuku_pos_time_;
    //int danmuku_max_count_;

    // ass config
    std::string chat_server_;
    std::string chat_id_;


} ass_config_t;


ass_config_t get_default_config();


} // namespace config

#endif // BILIBILI_DANMUKU_CONFIG_H
