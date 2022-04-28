#ifndef BILIBILI_DANMAKU_LIVE_MONITOR_H
#define BILIBILI_DANMAKU_LIVE_MONITOR_H

#include "live_danmaku.h"
#include "live_render_config.h"

#include "thirdparty/subprocess/subprocess.h"

class live_monitor {

  public:
    live_monitor()
        : danmaku_recv_count_(0), danmaku_render_count_(0), danmaku_time_(0),
          ass_render_time_(0), ffmpeg_time_(0), real_world_time_(0),
          real_world_time_base_(0), ffmpeg_process_handle_(nullptr),
          ffmpeg_output_handle_(nullptr), live_handle_(nullptr), room_id_(0),
          is_live_valid_(true){};

    void main_loop_thead();

    void live_status_monitor_thread();

    void ffmpeg_monitor_thread();

    void set_live_handle(live_danmaku *handle) {
        live_handle_ = handle;
    }

    void set_room_id(uint64_t id) {
        room_id_ = id;
    }

    void set_live_render_config(const config::live_render_config_t &cfg) {
        config_ = cfg;
    }

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

    void update_real_world_time(int time) {
        real_world_time_ = time;
    }

    void update_real_world_time_base(uint64_t time) {
        real_world_time_base_ = time;
    }

    void print_danmaku_inserted(int danmaku_count) const;

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
    int real_world_time_;
    uint64_t real_world_time_base_;

  private:
    FILE *ffmpeg_output_handle_;
    struct subprocess_s *ffmpeg_process_handle_;

    config::live_render_config_t config_;

    live_danmaku *live_handle_;
    uint64_t room_id_;
    bool is_live_valid_;
};

#endif //BILIBILI_DANMAKU_LIVE_MONITOR_H
