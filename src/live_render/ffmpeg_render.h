#ifndef BILIBILI_DANMAKU_FFMPEG_RENDER_H
#define BILIBILI_DANMAKU_FFMPEG_RENDER_H

#include <stdio.h>
#include <utility>

#include "danmaku.h"
#include "live_monitor.h"
#include "live_render_config.h"

#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

typedef struct image_t_ {
    int width, height, stride;
    unsigned char *buffer; // RGB24
} image_t;

class ffmpeg_render {

  public:
    // TODO: do not use config width and height
    ffmpeg_render(config::live_render_config_t &config, live_monitor *handle = nullptr)
        : config_(config), live_monitor_handle_(handle) {
        ass_img_.stride = config.video_width_ * 4;
        ass_img_.width = config.video_width_;

        // If the user only needs "move" type danmaku,
        // then we can safely reduce the height of the screen.
        if (config_.danmaku_pos_time_ == 0) {
            float height =
                static_cast<float>(config_.video_height_) * config_.danmaku_show_range_;
            height += static_cast<float>(config_.font_size_) *
                      config_.font_scale_; // add some margin
            ass_img_.height = static_cast<int>(height);
            config_.video_height_ = static_cast<int>(height);
        } else {
            ass_img_.height = config_.video_height_;
        }
        // Note that we use config video parameter to create ffmpeg subprocess.

        ass_img_.buffer = static_cast<unsigned char *>(
            malloc(ass_img_.height * ass_img_.width * 4 * 5));
        danmaku_queue_ = nullptr;
    }

    ~ffmpeg_render() {
        free(ass_img_.buffer);
    }

    void set_danmaku_queue(
        moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *p) {
        danmaku_queue_ = p;
    }

    void set_live_monitor_handle(live_monitor *handle) {
        live_monitor_handle_ = handle;
    }

    void main_thread();
    void run();

  private:
    live_monitor *live_monitor_handle_;
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *danmaku_queue_;
    image_t ass_img_;
    config::live_render_config_t config_;
};

#endif //BILIBILI_DANMAKU_FFMPEG_RENDER_H
