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
            sendSignal(ffmpeg_process_handle_->dwProcessId, SIGNAL_TYPE_CTRL_C);
        } catch (const std::exception &) {
        }
        
        std::this_thread::sleep_for(10s);
        // TODO: sleep and exit
#endif

        // TODO: Linux version
    }
}
void live_monitor::ffmpeg_monitor_thread() {
    assert(this->ffmpeg_process_handle_ != nullptr);
    assert(this->ffmpeg_output_handle_ != nullptr);

    std::thread([&]() {
        std::string ffmpeg_monitor_str(1024, 0);
        while (true) {
            if (!fgets(ffmpeg_monitor_str.data(), 1023, ffmpeg_output_handle_) ||
                ferror(ffmpeg_output_handle_) || feof(ffmpeg_output_handle_)) {
                break;
            }
            fmt::print("{}\n", ffmpeg_monitor_str.data());

            auto it_time = ffmpeg_monitor_str.find("time=");
            auto it_speed = ffmpeg_monitor_str.find("speed=");
            if (it_time != std::string::npos && it_speed != std::string::npos) {
                // time=00:00:19.32
                int _hour, _mins, _secs, _hundredths;
                sscanf(ffmpeg_monitor_str.data() + it_time + 5, "%d:%d:%d.%d", &_hour,
                       &_mins, &_secs, &_hundredths);

                ffmpeg_time_ = (60 * 60 * 1000) * _hour + (60 * 1000) * _mins +
                               (1000) * _secs + (10) * _hundredths;

                fmt::print(">com:{}<\n", ffmpeg_time_ - ass_render_time_);
            }
        }
        exit(0);
    }).detach();
}

void live_monitor::print_live_time() {
    fmt::print("danmaku:{}, render:{}, ffmpeg:{}\n", danmaku_time_, ass_render_time_,
               ffmpeg_time_);
}
