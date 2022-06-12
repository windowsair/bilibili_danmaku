#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "ass_danmaku.h"
#include "file_helper.h"

#include "git.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"
#endif

int main(int argc, char **argv) {

#if defined(_WIN32) || defined(_WIN64)
    // use utf8 codepage
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    fmt::print(fg(fmt::color::yellow),
               "xml2ass version {}, https://github.com/windowsair/bilibili_danmaku\n\n",
               GitMetadata::Describe());

    // get config
    auto user_config = config::get_user_ass_config();

    if (argc < 2) {
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::italic,
                   "用法: xml2ass <input_file>\n"
                   "例如: xml2ass 1.xml 2.xml\n");
        return -1;
    }

    if (user_config.use_custom_style_ &&
        !ass::is_custom_ass_file_exist("custom_style.ass")) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "已启用自定义样式，但custom_style.ass文件不存在\n");
        return -1;
    }

    std::vector<std::string> input_files;
    for (int i = 1; i < argc; i++) {
        input_files.push_back(argv[i]);
    }

    auto valid_file_list = file_helper::get_xml_file_list(input_files);

    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> count{static_cast<int>(valid_file_list.size())};
    int total_file_num = static_cast<int>(valid_file_list.size());

    auto job_start_time = std::chrono::high_resolution_clock::now();

    if (valid_file_list.size() > 1) {
        for (auto &item : valid_file_list) {
            std::thread([&]() {
                danmaku::DanmakuHandle handle;
                handle.danmaku_main_process(item, user_config);
                count--;
                cv.notify_all();
            }).detach();
        }

        // wait all jobs done.
        std::unique_lock<std::mutex> lk(m);
        while (count != 0) {
            cv.wait(lk, [&] { return count == 0; });
        }
    } else if (valid_file_list.size() == 1) {
        danmaku::DanmakuHandle handle;
        handle.danmaku_main_process(valid_file_list[0], user_config);
    }

    auto job_end_time = std::chrono::high_resolution_clock::now();
    double cost_time_ms =
        std::chrono::duration<double, std::milli>(job_end_time - job_start_time).count();
    fmt::print(fg(fmt::color::green), "完成。共{}个文件，用时{:.4f}s\n", total_file_num,
               cost_time_ms / 1000.0f);

    return 0;
}
