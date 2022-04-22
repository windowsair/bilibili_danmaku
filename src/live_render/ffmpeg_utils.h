#ifndef BILIBILI_DANMAKU_FFMPEG_UTILS_H
#define BILIBILI_DANMAKU_FFMPEG_UTILS_H

#include <chrono>

#include "live_danmaku.h"
#include "live_render_config.h"

#include "thirdparty/subprocess/subprocess.h"

void check_live_render_path(config::live_render_config_t &config);

bool init_stream_video_info(const std::vector<live_stream_info_t> &stream_list,
                            config::live_render_config_t &config);

void init_ffmpeg_subprocess(struct subprocess_s *subprocess,
                            config::live_render_config_t &config);

// get timestamp in ms
inline uint64_t get_now_timestamp() {
    const auto p1 = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch())
        .count();
}


#endif //BILIBILI_DANMAKU_FFMPEG_UTILS_H
