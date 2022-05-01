#if defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cassert>
#include <chrono>
#include <thread>

#include "ffmpeg_utils.h"
#include "live_monitor.h"

#include "thirdparty/fmt/include/fmt/color.h"

#if defined(_WIN32) || defined(_WIN64)
#include "thirdparty/windows-kill-library/windows-kill-library.h"
#endif

void live_monitor::stop_ffmpeg_record() {
    using namespace std::chrono_literals;

    if (ffmpeg_process_handle_) {
#if defined(_WIN32) || defined(_WIN64)
        using WindowsKillLibrary::sendSignal;
        using WindowsKillLibrary::SIGNAL_TYPE_CTRL_BREAK;
        using WindowsKillLibrary::SIGNAL_TYPE_CTRL_C;

        try {
            sendSignal(static_cast<DWORD>(ffmpeg_process_handle_->dwProcessId),
                       SIGNAL_TYPE_CTRL_C);
        } catch (const std::exception &) {
        } // TODO: not throw

        std::this_thread::sleep_for(5s);

#else
        subprocess_destroy(ffmpeg_process_handle_);
        std::this_thread::sleep_for(5s);
#endif
    }
}

void live_monitor::exit_live_render() {
    force_exit_mutex_.lock();
    std::quick_exit(0);
}

void live_monitor::live_status_monitor_thread() {
    using namespace std::chrono_literals;

    assert(this->live_handle_ != nullptr);

    std::thread([&]() {
        uint64_t live_end_time;

        while (true) {
            // Note: The user needs to ensure that the request will not always fail.
            std::this_thread::sleep_for(30s);
            auto room_detail = this->live_handle_->get_room_detail(this->room_id_);
            if (room_detail.code_ == -1) {
                continue; // req failed.
            }

            if (room_detail.live_status_ != live_detail_t::VALID) {
                live_end_time = get_now_timestamp() - this->real_world_time_base_;

                std::this_thread::sleep_for(20s);
                // retry
                room_detail = this->live_handle_->get_room_detail(this->room_id_);
                if (room_detail.code_ == -1) {
                    continue; // req failed
                }
                if (room_detail.live_status_ != live_detail_t::VALID) {
                    break;
                }
            }
        }

        // let's wait ffmpeg done
        uint64_t last_ffmpeg_time = this->ffmpeg_time_;
        int time_count = 0;
        while (time_count < 6) { // timeout: 1min
            if (this->ffmpeg_time_ > live_end_time) {
                break;
            }

            if (last_ffmpeg_time != this->ffmpeg_time_) {
                time_count = 0;
            }

            last_ffmpeg_time = this->ffmpeg_time_;

            std::this_thread::sleep_for(10s);
            time_count++;
        }

        fmt::print(fg(fmt::color::yellow), "直播已结束\n");

        this->is_live_valid_ = false;
        this->stop_ffmpeg_record();

        // this may not happen....
        std::this_thread::sleep_for(30s);
        this->exit_live_render();

        // TODO: clean up
    }).detach();
}

void live_monitor::ffmpeg_monitor_thread() {
    using namespace std::chrono_literals;

    if (this->config_.verbose_ &
        static_cast<int>(config::systemVerboseMaskEnum::NO_STAT_INFO)) {
        return;
    }

    std::thread([&]() {
        while (this->is_live_valid_) {
            if (this->ffmpeg_time_ > 0)
                this->print_live_time();
            std::this_thread::sleep_for(5s);
        }
    }).join();
}

void live_monitor::print_live_time() {
    fmt::print("real:{}, danmaku:{}, render:{}, ffmpeg:{}\n", real_world_time_,
               danmaku_time_, ass_render_time_, ffmpeg_time_);
}

void live_monitor::print_danmaku_inserted(int danmaku_count) const {
    if (!(config_.verbose_ &
          static_cast<int>(config::systemVerboseMaskEnum::NO_DANMAKU))) {
        fmt::print("已经装填弹幕[{}]\n", danmaku_count);
    }
}
