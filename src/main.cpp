#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "ass.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

int main(int argc, char **argv) {

    if (argc < 2) {
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::italic,
                   "Usage: xml2ass <input_file>\n "
                   "example: xml2ass 1.xml 2.xml\n");
    }

    std::vector<std::string> input_files;
    for (int i = 1; i < argc; i++) {
        input_files.push_back(argv[i]);
    }

    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> count{static_cast<int>(input_files.size())};

    auto job_start_time = std::chrono::high_resolution_clock::now();

    for (auto &item : input_files) {
        std::thread([&]() {
            danmuku::danmuku_main_process(item);
            count--;
            cv.notify_all();
        }).detach();
    }

    // wait all jobs done.
    std::unique_lock<std::mutex> lk(m);
    while (count != 0) {
        cv.wait(lk, [&] { return count == 0; });
    }

    auto job_end_time = std::chrono::high_resolution_clock::now();
    double cost_time_ms =
        std::chrono::duration<double, std::milli>(job_end_time - job_start_time).count();
    fmt::print(fg(fmt::color::green), "完成.用时{:.4f}s\n", cost_time_ms / 1000.0f);

    return 0;
}
