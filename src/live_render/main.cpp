#include <cassert>
#include <iostream>
#include <thread>

#include "ffmpeg_render.h"
#include "live_danmaku.h"
#include "live_monitor.h"

#include "ass_config.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

#include <stdio.h>
#include <windows.h>

live_monitor *kLive_monitor_handle = nullptr;

BOOL WINAPI consoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT)
        printf("Ctrl-C handled\n"); // do cleanup

    if (kLive_monitor_handle) {
        kLive_monitor_handle->stop_ffmpeg_record();
    }
    return TRUE;
}

int main() {
    live_monitor global_monitor;
    kLive_monitor_handle = &global_monitor;

    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        printf("\nERROR: Could not set control handler");
        return 1;
    }

    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> queue(100);

    live_danmaku live;
    live.set_danmaku_queue(&queue);

    auto room_detail = live.get_room_detail(22634198);
    // TODO: verify

    int room_id = room_detail.room_id_;
    auto address_list = live.get_live_room_stream(room_id, 20000);
    if (address_list.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "无法获取直播地址\n");
    }

    auto config = config::get_user_config();
    ffmpeg_render s(config);
    s.set_danmaku_queue(&queue);
    s.set_live_monitor_handle(&global_monitor);

    // TODO: try addres list
    s.set_ffmpeg_input_address(address_list[0]);

    s.main_thread();

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    live.run(room_detail.room_detail_str_);

    while (1) {
        ;
    }

    return 0;
}
