#ifndef BILIBILI_DANMAKU_FFMPEG_RENDER_H
#define BILIBILI_DANMAKU_FFMPEG_RENDER_H

#include <stdio.h>
#include <utility>

#include "ass_config.h"
#include "danmaku.h"
#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

typedef struct image_t_ {
    int width, height, stride;
    unsigned char *buffer; // RGB24
} image_t;

class ffmpeg_render {

  public:
    // TODO: do not use config width and height
    ffmpeg_render(config::ass_config_t &config) : config_(config) {
        ass_img_.stride = config.video_width_ * 4;
        ass_img_.width = config.video_width_;
        ass_img_.height = config.video_height_;
        ass_img_.buffer =
            static_cast<unsigned char *>(malloc(ass_img_.height * ass_img_.width * 4));
        ffmpeg_pipe_ = nullptr;
        danmaku_queue_ = nullptr;
    }

    ~ffmpeg_render() {
        free(ass_img_.buffer);
    }

    void set_danmaku_queue(
        moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *p) {
        danmaku_queue_ = p;
    }

    void set_ffmpeg_input_address(std::string s) {
        ffmpeg_input_address_ = s;
    }

    void main_thread();
    void run();

  private:
    FILE *ffmpeg_pipe_;
    std::string ffmpeg_input_address_;
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *danmaku_queue_;
    image_t ass_img_;
    config::ass_config_t config_;
};

#endif //BILIBILI_DANMAKU_FFMPEG_RENDER_H
