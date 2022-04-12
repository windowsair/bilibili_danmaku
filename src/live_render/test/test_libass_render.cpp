#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <vector>

#include "../ffmpeg_render.h"
#include "../live_danmaku.h"

#include "ass_config.h"
#include "danmaku.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

using namespace moodycamel;
using namespace danmaku;

void wait_forever() {
    std::condition_variable cv;
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [] { return false; });
}

int main() {
    std::string xml_file = "D:/out.xml";

    auto config = config::get_user_ass_config();

    ffmpeg_render s(config);

    ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> danmaku_queue(100);
    s.set_danmaku_queue(&danmaku_queue);
    s.set_ffmpeg_input_address("K:/ff/1.flv");
    s.main_thread();

    // process danmaku

    std::vector<danmaku_item_t> danmaku_all_list, danmaku_move_list, danmaku_pos_list;
    danmaku_info_t danmaku_info;
    pugi::xml_document doc;
    pugi::xml_parse_result parse_result;

    DanmakuHandle handle;
    int ret = handle.parse_danmaku_xml(doc, parse_result, xml_file, danmaku_all_list,
                                       danmaku_info);
    if (ret != 0) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:无效的XML文件\n",
                   xml_file);
        return -1;
    }
    if (danmaku_all_list.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:未找到有效项目\n",
                   xml_file);
        return -1;
    }

    handle.process_danmaku_list(danmaku_all_list, danmaku_move_list, danmaku_pos_list);

    std::vector<danmaku::danmaku_item_t> insert_list;
    auto last = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();

    // process queue
    size_t i, j;
    for (i = 0; i < danmaku_move_list.size() - 1;) {
        for (j = i + 1; j < danmaku_move_list.size(); j++) {
            // get same second item
            int a = danmaku_move_list[i].start_time_;
            int b = danmaku_move_list[j].start_time_;

            if (a != b) {
                break;
            }
        }

        insert_list.clear();
        // [a, b)
        for (auto k = i; k < j; k++) {
            insert_list.push_back(danmaku_move_list[k]);
        }
        if (!insert_list.empty()) {
            // wait
            do {
                now = std::chrono::high_resolution_clock::now();
            } while (std::chrono::duration<double, std::milli>(now - last).count() <
                     200); // 200ms delay

            danmaku_queue.enqueue(insert_list);

            last = std::chrono::high_resolution_clock::now();
        }

        i = j;
    }

    wait_forever();
    return 0;
}