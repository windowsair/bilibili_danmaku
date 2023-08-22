#if defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cassert>
#include <string>
#include <vector>

#include "test_sc_common.h"
#include "ass_render_utils.h"
#include "file_helper.h"
#include "live_render_config.h"
#include "sc_control.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

#define USE_AEGISUB_OVERLAY 1

inline std::string kFileName;
constexpr int VIDEO_HEIGHT = 1080;
constexpr int VIDEO_WIDTH = 1920;

class sc::ScControlDebugProbe {
  public:
    void check_render_done_status(sc::ScControl *control) {
        assert(control->sc_list_.size() == 0);
        assert(control->wait_in_deque_.size() == 0);
        assert(control->show_deque_.size() == 0);
        assert(control->fade_out_deque_.size() == 0);
    }
};

struct debug_render_control : public ass::sc_ass_render_control {
  public:
    FILE *fd_;
    int layer_;
    std::string layer_start_time_;
    bool need_refresh_;

    debug_render_control() : sc_ass_render_control(nullptr) {
        layer_ = 1;
        need_refresh_ = false;
        fd_ = fopen(kFileName.c_str(), "wb+");
    };

    ~debug_render_control() {
        fflush(fd_);
        fclose(fd_);
    }

    void create_track(char *ass_style_header, size_t ass_style_header_length) override {
        fmt::print(fd_, ass_style_header);
    }

    void refresh_background_layer(std::string &str) {
        int dummy, hours, mins, seconds, ms;
        int ret;
        ret = sscanf(str.c_str(), "Dialogue: %d,%d:%d:%d.%d", &dummy, &hours, &mins,
                     &seconds, &ms);
        assert(ret == 5);


#if defined USE_AEGISUB_OVERLAY && (USE_AEGISUB_OVERLAY == 1)
        fmt::print(fd_,
                   "Dialogue: "
                   "{},{}:{}:{}.{},9:00:00.00,Default,,0,0,0,,{{\\p1\\1c&HFFFFFF&\\alpha&"
                   "H00&}}m "
                   "0 0 l 1920 0 l 1920 1080 l 0 1080{{\\p0}}\n",
                   layer_++, hours, mins, seconds, ms);
#endif


        fflush(fd_);

        need_refresh_ = false;
    }

    void update_event_line(std::vector<std::string> &ass_list) override {
        int length = strlen("Dialogue: 1");
        int offset;
        std::string str;
        std::string res;
        std::string find_str;

        for (auto &item : ass_list) {
            str += item;
        }

#if defined USE_AEGISUB_OVERLAY && (USE_AEGISUB_OVERLAY == 1)
        if (need_refresh_) {
            refresh_background_layer(str);
        }

        if (auto pos = str.find("Dialogue: 0"); pos != std::string::npos) {
            find_str = "Dialogue: 0";
            offset = 0;
        } else if (auto pos = str.find("Dialogue: 1"); pos != std::string::npos) {
            find_str = "Dialogue: 1";
            offset = 1;
        } else {
            fmt::print("Wrong ass output\n");
            std::abort();
        }

        while (true) {
            size_t pos1 = str.find("Dialogue: 0");
            size_t pos2 = str.find("Dialogue: 1");
            size_t pos;

            if (pos1 == std::string::npos && pos2 == std::string::npos) {
                break;
            } else if (pos1 < pos2) {
                offset = 0;
                pos = pos1;
            } else if (pos2 < pos1) {
                offset = 1;
                pos = pos2;
            } else {
                std::abort();
            }

            res += str.substr(0, pos) + fmt::format("Dialogue: {}", layer_ + offset);
            str = str.substr(pos + length);
            pos = str.find(find_str);
        }
#endif

        res += str;


        fmt::print(fd_, "{}", res);
        fflush(fd_);
    }

    void flush_track() override {
        layer_ += 3;
        need_refresh_ = true;
    }
};

void sc_control_process(moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> *p,
                        config::live_render_config_t &config, int max_alive_time) {
    constexpr int fps = 60;
    double tm = 0;
    double step = ((double)(1000) / (double)(fps));
    double double_tm = 0;
    double double_sc_tm = 0;

    debug_render_control render_probe;

    sc::ScControl sc_control{&render_probe, config};

    config::live_render_config_t copy_cfg = config;
    copy_cfg.font_size_ = copy_cfg.sc_font_size_;
    copy_cfg.font_alpha_ = copy_cfg.sc_alpha_;
    //copy_cfg.video_height_ = VIDEO_HEIGHT;
    copy_cfg.video_width_ = VIDEO_WIDTH;

    std::vector<danmaku::ass_dialogue_t> ass_dialogue_list;
    auto sc_ass_header_str = ass::get_sc_ass_header(copy_cfg, ass_dialogue_list);
    render_probe.create_track((char *)sc_ass_header_str.c_str(),
                              sc_ass_header_str.length());

    while (true) {
        if (tm > max_alive_time) {
            break;
        }

        {
            int sc_end_time;

            for (int i = 0; i < 5; i++) {
                double_sc_tm += step;
            }
            sc_end_time = static_cast<int>(double_sc_tm);

            sc_control.updateSuperChatEvent(&render_probe, config, tm, sc_end_time - tm,
                                            p, nullptr);
        }

        double_tm += step;
        tm = static_cast<int>(double_tm);

        double_tm += step;
        tm = static_cast<int>(double_tm);

        double_tm += step;
        tm = static_cast<int>(double_tm);

        double_tm += step;
        tm = static_cast<int>(double_tm);

        double_tm += step;
        tm = static_cast<int>(double_tm);
    }

    sc::ScControlDebugProbe debug_checker;
    debug_checker.check_render_done_status(&sc_control);
}

int main() {
    std::vector<std::string> input_files = {"./sc_list/"};
    std::vector<std::string> xml_list = file_helper::get_xml_file_list(input_files);
    if (xml_list.empty()) {
        return -1;
    }

    auto config = config::get_user_live_render_config();
    config.video_height_ = 1080;

    for (auto &item : xml_list) {
        moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> queue;
        int max_alive_time;

        kFileName = item + ".ass";
        max_alive_time = process_sc_list_from_file(item, queue);
        sc_control_process(&queue, config, max_alive_time);
    }

    fmt::print("done\n");
    return 0;
}
