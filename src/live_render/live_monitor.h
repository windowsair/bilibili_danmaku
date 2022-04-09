#ifndef BILIBILI_DANMAKU_LIVE_MONITOR_H
#define BILIBILI_DANMAKU_LIVE_MONITOR_H

#include "thirdparty/subprocess/subprocess.h"

class live_monitor {

  public:
    live_monitor()
        : danmaku_recv_count_(0), danmaku_render_count_(0),
          ffmpeg_process_handle_(nullptr), ffmpeg_output_handle_(nullptr){};

    void main_loop_thead();

    void live_status_monitor_thread();

    void ffmpeg_monitor_thread();

    void set_ffmpeg_process_handle(struct subprocess_s *handle) {
        ffmpeg_process_handle_ = handle;
    }

    void set_ffmpeg_output_handle(FILE *handle) {
        ffmpeg_output_handle_ = handle;
    }

    void update_danmaku_time(int time) {
        danmaku_time_ = time;
    }
    void update_ass_render_time(int time) {
        ass_render_time_ = time;
    }
    void update_ffmpeg_time(int time) {
        ffmpeg_time_ = time;
    }

    void print_live_time();

    void stop_ffmpeg_record();

  public:
    // stat info
    int danmaku_recv_count_;
    int danmaku_render_count_; // ? what is this

    // time stat info
    int danmaku_time_;
    int ass_render_time_;
    int ffmpeg_time_;

  private:
    FILE *ffmpeg_output_handle_;
    struct subprocess_s *ffmpeg_process_handle_;
};

#endif //BILIBILI_DANMAKU_LIVE_MONITOR_H
