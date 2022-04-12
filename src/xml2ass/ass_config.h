#ifndef BILIBILI_DANMAKU_ASS_CONFIG_H
#define BILIBILI_DANMAKU_ASS_CONFIG_H

#include <string>

namespace config {
typedef struct ass_config_ {
  public:
    // user config

    // video parameter config
    int video_width_;
    int video_height_;

    // font config
    std::string font_family_;
    int font_color_; // user not change
    int font_size_;  // user not change
    float font_scale_;
    float font_alpha_;
    bool font_bold_;

    // danmaku config
    float danmaku_show_range_;
    int danmaku_move_time_;
    int danmaku_pos_time_;
    //int danmaku_max_count_;

    // ass config
    std::string chat_server_;
    std::string chat_id_;

} ass_config_t;

ass_config_t get_user_ass_config();

} // namespace config

#endif // BILIBILI_DANMAKU_ASS_CONFIG_H
