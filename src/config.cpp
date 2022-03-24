#include "config.h"

namespace config {

ass_config_t get_default_config() {
    ass_config_t config = {
        .video_width_ = 672,
        .video_height_ = 504,
        .font_family_ = "微软雅黑",
        .font_color_ = 0xFFFFFF, // white
        .font_size_ = 1,
        .font_scale_ = 1.6f,
        .font_alpha_ = 0.50f,
        .font_bold_ = true,
        .danmuku_show_range_ = 0.5f,
        .danmuku_move_time_ = 12,
        .danmuku_pos_time_ = 5,
    };

    return config;
}

} // namespace config