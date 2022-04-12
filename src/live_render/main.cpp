#include <cassert>
#include <iostream>
#include <thread>

#include "ffmpeg_render.h"
#include "ffmpeg_utils.h"

#include "live_danmaku.h"
#include "live_monitor.h"

#include "live_render_config.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

#include <stdio.h>
#include <windows.h>

inline live_monitor *kLive_monitor_handle = nullptr;

#if defined(_WIN32) || defined(_WIN64)
BOOL WINAPI consoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT)
        printf("Ctrl-C handled\n"); // do cleanup

    if (kLive_monitor_handle) {
        kLive_monitor_handle->stop_ffmpeg_record();
    }
    return TRUE;
}
#endif

void set_console_handle() {
#if defined(_WIN32) || defined(_WIN64)
    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "ERROR: Could not set control handler\n");
        std::abort();
    }
#endif
}

int main(int argc, char **argv) {
    using namespace std::chrono_literals;

    // TODO: extract
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    live_monitor global_monitor;
    kLive_monitor_handle = &global_monitor;

    set_console_handle();

    auto config = config::get_user_live_render_config();

    check_live_render_path(config);

    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> queue(100);
    live_danmaku live;
    live.set_danmaku_queue(&queue);

    // step1: get live info
    // TODO: parameter
    auto room_id = 5050;
    auto room_detail = live.get_room_detail(room_id);

    if (room_detail.live_status_ != live_detail_t::VALID) {
        fmt::print(fg(fmt::color::yellow), "暂未开播，等待中...");

        while (room_detail.live_status_ != live_detail_t::VALID) {
            std::this_thread::sleep_for(30s);
            room_detail = live.get_room_detail(room_id);
        }
    }

    if (room_detail.room_detail_str_.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取直播间信息失败");
        std::abort();
    }

    config.user_uid_ = room_detail.user_uid_;

    auto live_title = live.get_live_room_title(room_detail.user_uid_);
    config.filename_ = live_title;

    auto stream_list = live.get_live_room_stream(room_detail.room_id_, 20000);
    // get stream meta info and update config.
    init_stream_video_info(stream_list, config);

    //
    //
    ffmpeg_render render(config, &global_monitor);
    render.set_danmaku_queue(&queue);

    // start ffmpeg render
    render.main_thread();

    std::this_thread::sleep_for(1s);

    // capture live danmaku
    live.run(room_detail.room_detail_str_);

    while (1) {
        ;
    }

    return 0;
}
