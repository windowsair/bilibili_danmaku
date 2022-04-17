#if defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cassert>
#include <chrono>
#include <thread>

#include "live_monitor.h"

#include "thirdparty/fmt/include/fmt/color.h"

#if defined(_WIN32) || defined(_WIN64)
#include "thirdparty/windows-kill-library/windows-kill-library.h"
#endif

void live_monitor::stop_ffmpeg_record() {
    if (ffmpeg_process_handle_) {
#if defined(_WIN32) || defined(_WIN64)
        using WindowsKillLibrary::sendSignal;
        using WindowsKillLibrary::SIGNAL_TYPE_CTRL_BREAK;
        using WindowsKillLibrary::SIGNAL_TYPE_CTRL_C;
        using namespace std::chrono_literals;

        try {
            sendSignal(static_cast<DWORD>(ffmpeg_process_handle_->dwProcessId),
                       SIGNAL_TYPE_CTRL_C);
        } catch (const std::exception &) {
        } // TODO: not throw

        std::this_thread::sleep_for(5s);

#else
        subprocess_destroy(&subprocess);
        std::this_thread::sleep_for(5s);
#endif
    }
}

void live_monitor::live_status_monitor_thread() {
    using namespace std::chrono_literals;

    assert(this->live_handle_ != nullptr);

    std::thread([&]() {
        while (true) {
            std::this_thread::sleep_for(30s);
            auto room_detail = this->live_handle_->get_room_detail(this->room_id_);

            if (room_detail.live_status_ != live_detail_t::VALID) {
                std::this_thread::sleep_for(20s);
                // retry
                room_detail = this->live_handle_->get_room_detail(this->room_id_);
                if (room_detail.live_status_ != live_detail_t::VALID) {
                    break;
                }
            }
        }

        fmt::print(fg(fmt::color::yellow), "直播已结束\n");

        this->is_live_valid_ = false;
        this->stop_ffmpeg_record();

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
    fmt::print("danmaku:{}, render:{}, ffmpeg:{}\n", danmaku_time_, ass_render_time_,
               ffmpeg_time_);
}

void live_monitor::print_danmaku_inserted(int danmaku_count) const {
    if (!(config_.verbose_ &
          static_cast<int>(config::systemVerboseMaskEnum::NO_DANMAKU))) {
        fmt::print("已经装填弹幕[{}]\n", danmaku_count);
    }
}
