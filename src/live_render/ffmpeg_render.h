#ifndef BILIBILI_DANMAKU_FFMPEG_RENDER_H
#define BILIBILI_DANMAKU_FFMPEG_RENDER_H

#include <stdio.h>
#include <utility>

#include "danmaku.h"
#include "live_danmaku.h"
#include "live_monitor.h"
#include "live_render_config.h"

#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

typedef struct image_t_ {
    int width, height, stride;
    unsigned char *buffer; // RGB24
} image_t;

class ffmpeg_render {

  public:
    ffmpeg_render(config::live_render_config_t &config,
                  live_monitor *live_monitor_handle = nullptr,
                  live_danmaku *live_danmaku_handle = nullptr)
        : config_(config), live_monitor_handle_(live_monitor_handle),
          live_danmaku_handle_(live_danmaku_handle), danmaku_queue_(nullptr),
          sc_queue_(nullptr) {
        ass_img_.stride = config.video_width_ * 4;
        ass_img_.width = config.video_width_;

        // If the user only needs "move" type danmaku,
        // and they don't want to record super chat message,
        // then we can safely reduce the height of the screen.
        // Note that sc and danmaku share the same image buffer, even though their
        // layers may be different.
        if (config_.sc_enable_ == false && config_.danmaku_pos_time_ == 0) {
            float height =
                static_cast<float>(config_.video_height_) * config_.danmaku_show_range_;
            height += static_cast<float>(config_.font_size_) *
                      config_.font_scale_; // add some margin
            ass_img_.height = static_cast<int>(height);
            config_.video_height_ = static_cast<int>(height);

            // The height has been modified, so all of them can be displayed now
            config_.danmaku_show_range_ = 1.0f;
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

    void set_sc_queue(moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> *p) {
        sc_queue_ = p;
    }

    void set_live_monitor_handle(live_monitor *handle) {
        live_monitor_handle_ = handle;
    }

    void main_thread();
    void run();

  private:
    live_danmaku *live_danmaku_handle_;
    live_monitor *live_monitor_handle_;
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *danmaku_queue_;
    moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> *sc_queue_;
    image_t ass_img_;
    config::live_render_config_t config_;
};

#endif //BILIBILI_DANMAKU_FFMPEG_RENDER_H
