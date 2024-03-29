﻿#ifndef BILIBILI_DANMAKU_LIVE_RENDER_CONFIG_H
#define BILIBILI_DANMAKU_LIVE_RENDER_CONFIG_H

#include "ass_config.h"

namespace config {

enum systemVerboseMaskEnum {
    NO_FFMPEG = 1,
    NO_DANMAKU = 2,
    NO_STAT_INFO = 4,
};

enum verticalProcessEnum {
    DEFAULT = 0,
    DROP = 1,
    CONVERT = 2,
};

typedef struct live_render_config_ : public ass_config_t {
    // ffmpeg setting
    std::string ffmpeg_path_;
    std::string output_file_path_;
    bool post_convert;

    // video setting
    int fps_;
    std::string video_bitrate_;
    std::string audio_bitrate_;
    std::string decoder_;
    std::string encoder_;
    std::vector<std::string> extra_encoder_info_;
    std::string extra_input_stream_info_;
    std::string extra_filter_info_;

    uint64_t segment_time_;

    // ffmpeg setting
    int ffmpeg_thread_queue_size_;
    int render_thread_queue_size_;

    int danmaku_lead_time_compensation_;

    // system setting
    int verbose_;
    int vertical_danmaku_strategy_;

    // stream setting
    std::string filename_;
    std::string stream_address_;

    std::string actual_file_name_; // In some cases, the file name may not legal,
                                   // we will store the actual file name here.
                                   // This may be the same as the original file name,
                                   // or it may be a timestamp format.
                                   // The encoding of this field is always UTF8

    std::string bilibili_proxy_address_;
    std::string bilibili_cookie_;

    // origin input stream info
    int origin_video_width_;
    int origin_video_height_;
    int origin_video_fps_;

    // adjust input stream info
    int adjust_video_width_;
    int adjust_video_height_;
    int adjust_video_fps_;

    // super chat
    bool sc_enable_;
    int sc_font_size_;
    float sc_show_range_;
    float sc_alpha_;
    int sc_max_width_;
    int sc_margin_x_;
    bool sc_y_mirror_;
    bool sc_price_no_break_;

    // live info
    uint64_t user_uid_;
    int live_stream_format_;

    live_render_config_() {
        sc_enable_ = false;
        sc_price_no_break_ = false;
    }
    live_render_config_(const ass_config_t &cfg) : ass_config_t(cfg) {
        sc_enable_ = false;
        sc_price_no_break_ = false;
    }

} live_render_config_t;

live_render_config_t get_user_live_render_config();

} // namespace config

#endif //BILIBILI_DANMAKU_LIVE_RENDER_CONFIG_H
