/**
 *  Process danmaku recv, ass render and ffmpeg overlay sender
 */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <string>
#include <thread>

#include "ass_danmaku.h"
#include "ffmpeg_render.h"
#include "ffmpeg_utils.h"
#include "live_monitor.h"

#include "thirdparty/libass/include/ass.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

#include "thirdparty/subprocess/subprocess.h"

inline int kDanmaku_inserted_count = 0;
inline volatile int kFfmpeg_output_time = 0; // time in ms

extern "C" {
int ass_process_events_line(ASS_Track *track, char *str);
}

#define TO_R(c) ((c) >> 24)
#define TO_G(c) (((c) >> 16) & 0xFF)
#define TO_B(c) (((c) >> 8) & 0xFF)
#define TO_A(c) ((c)&0xFF)

// RGBA img blend
inline void blend_single(image_t *frame, ASS_Image *img, uint64_t offset) {
    int x, y;
    unsigned char opacity = 255 - TO_A(img->color);
    unsigned char r = TO_R(img->color);
    unsigned char g = TO_G(img->color);
    unsigned char b = TO_B(img->color);

    unsigned char *src;
    unsigned char *dst;

    src = img->bitmap;
    dst = (frame->buffer + offset) + img->dst_y * frame->stride + img->dst_x * 4;
    for (y = 0; y < img->h; ++y) {
        for (x = 0; x < img->w; ++x) {
            uint32_t k = ((uint32_t)src[x]) * opacity;
            // possible endianness problems...
            // would anyone actually use big endian machine??
            dst[x * 4] = (k * r + (255 * 255 - k) * dst[x * 4]) / (255 * 255);
            dst[x * 4 + 1] = (k * g + (255 * 255 - k) * dst[x * 4 + 1]) / (255 * 255);
            dst[x * 4 + 2] = (k * b + (255 * 255 - k) * dst[x * 4 + 2]) / (255 * 255);
            dst[x * 4 + 3] = (k * 255 + (255 * 255 - k) * dst[x * 4 + 3]) / (255 * 255);
        }
        src += img->stride;
        dst += frame->stride;
    }
}

inline void blend(image_t *frame, ASS_Image *img, uint64_t offset) {
    while (img) {
        blend_single(frame, img, offset);
        img = img->next;
    }
}

void libass_msg_callback(int level, const char *fmt, va_list va, void *data) {
    if (level > 6)
        return;
    printf("libass: ");
    vprintf(fmt, va);
    printf("\n");
}

void libass_no_msg_callback(int level, const char *fmt, va_list va, void *data) {
}

inline void libass_init(ASS_Library **ass_library, ASS_Renderer **ass_renderer,
                        int frame_w, int frame_h, bool enable_message_output) {
    *ass_library = ass_library_init();
    if (!(*ass_library)) {
        printf("ass_library_init failed!\n");
        std::abort();
    }

    if (enable_message_output) {
        ass_set_message_cb(*ass_library, libass_msg_callback, NULL);
    } else {
        ass_set_message_cb(*ass_library, libass_no_msg_callback, NULL);
    }

    ass_set_extract_fonts(*ass_library, 1);

    *ass_renderer = ass_renderer_init(*ass_library);
    if (!(*ass_renderer)) {
        printf("ass_renderer_init failed!\n");
        std::abort();
    }

    ass_set_frame_size(*ass_renderer, frame_w, frame_h);
    ass_set_fonts(*ass_renderer, NULL, "sans-serif", ASS_FONTPROVIDER_AUTODETECT, NULL,
                  1);
}

inline void wait_queue_ready(
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *queue) {

    using namespace std::chrono_literals;

    auto p = queue->peek();
    int count = 0;
    while (p == nullptr && count < 3000 / 10) {
        std::this_thread::sleep_for(10ms);
        count++;
        p = queue->peek();
    }
}

inline void update_libass_event(
    ASS_Track *ass_track, danmaku::DanmakuHandle &handle, config::ass_config_t &config,
    int base_time, bool is_base_time_lag,
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *queue,
    live_monitor *monitor) {

    // TODO:
    // We should ensure that the list is sorted in ascending chronological order.

    static bool restart = false;
    static int last_restart_time = 0;

    while (auto p = queue->peek()) {
        std::vector<danmaku::danmaku_item_t> &danmaku_list = *p;
        assert(!danmaku_list.empty());
        static int delay_count = 0;

        // get minimum start time
        float min_element_time =
            (std::min_element(
                 danmaku_list.begin(), danmaku_list.end(),
                 [](const danmaku::danmaku_item_t &a, const danmaku::danmaku_item_t &b) {
                     return a.start_time_ < b.start_time_;
                 }))
                ->start_time_;

        int min_start_time = min_element_time * 1000;

        bool render_danmaku = false;
        if (min_start_time < base_time) {
            // TODO: When lag occurs, we should use the newest danmaku.
            if ((is_base_time_lag && base_time - min_start_time < 6 * 1000) ||
                (!is_base_time_lag && base_time - min_start_time < 30 * 1000)
                // allow rendering of danmaku that appear earlier
                // (!REMOVED!)force the timeline to be calibrated just at the beginning (base_time - min_start_time < 30 * 1000 && base_time < 60 * 2 * 1000)
            ) {
                // The danmaku appear behind the current rendering time.
                // If the time difference is relatively short, then we pull the base time
                // of these danmaku after the current base time and render them.
                int offset_time_ = base_time - min_start_time + 10;
                float offset_time = offset_time_ / 1000.0f;
                for (auto &item : danmaku_list) {
                    item.start_time_ += offset_time;
                }
                render_danmaku = true;
            }
        } else {
            // The rendering can't keep up for now, so just insert danmaku directly.
            render_danmaku = true;
        }

        if (render_danmaku) {
            for (auto &item : danmaku_list) {
                item.update_context();
            }

            std::vector<danmaku::ass_dialogue_t> ass_dialogue_list;

            if (config.danmaku_pos_time_ > 0) {
                std::vector<std::reference_wrapper<danmaku::danmaku_item_t>>
                    pos_danmaku_ref_list;
                std::vector<std::reference_wrapper<danmaku::danmaku_item_t>>
                    move_danmaku_ref_list;

                for (auto &item : danmaku_list) {
                    if (item.danmaku_type_ ==
                        static_cast<int>(danmaku::danmaku_type::MOVE)) {
                        move_danmaku_ref_list.push_back(std::ref(item));
                    } else {
                        pos_danmaku_ref_list.push_back(std::ref(item));
                    }
                }

                handle.process_danmaku_dialogue_move(move_danmaku_ref_list, config,
                                                     ass_dialogue_list);
                handle.process_danmaku_dialogue_pos(pos_danmaku_ref_list, config,
                                                    ass_dialogue_list);
            } else {

                // The sender promises not to add pos type danmaku. So we can handle it directly.
                handle.process_danmaku_dialogue_move(danmaku_list, config,
                                                     ass_dialogue_list);
            }

            for (auto &item : ass_dialogue_list) {
                // convert to event_str
                std::string event_str = ass::get_ass_event(config, item);
                event_str.push_back('\0');
                // insert danmaku
                ass_process_events_line(ass_track, event_str.data());
            }

            kDanmaku_inserted_count += ass_dialogue_list.size();

            monitor->print_danmaku_inserted(kDanmaku_inserted_count);
            monitor->update_danmaku_time(min_start_time);
        }

        queue->pop();
    }
}

void ffmpeg_render::run() {
    using namespace fmt::literals;

    // pre check

    if (this->danmaku_queue_ == nullptr) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "内部错误：未设置弹幕队列\n");
        std::abort();
    }
    assert(this->live_monitor_handle_ != nullptr);

    // init libass

    ASS_Library *ass_library = nullptr;
    ASS_Renderer *ass_renderer = nullptr;

    bool enable_libass_output =
        !(config_.verbose_ & static_cast<int>(config::systemVerboseMaskEnum::NO_FFMPEG));

    libass_init(&ass_library, &ass_renderer, config_.video_width_, config_.video_height_,
                enable_libass_output);

    std::vector<danmaku::ass_dialogue_t> ass_dialogue_list;

    std::string ass_header_str = ass::get_ass_header(config_, ass_dialogue_list);

    ASS_Track *ass_track =
        ass_read_memory(ass_library, const_cast<char *>(ass_header_str.c_str()),
                        ass_header_str.size(), NULL);

    // create ffmpeg subprocess
    struct subprocess_s subprocess;

    init_ffmpeg_subprocess(&subprocess, config_);

    this->live_monitor_handle_->set_live_render_config(config_);
    this->live_monitor_handle_->set_ffmpeg_process_handle(&subprocess);

    FILE *ffmpeg_ = subprocess_stdin(&subprocess);
    FILE *p_stderr = subprocess_stderr(&subprocess);
    if (ffmpeg_ == nullptr || p_stderr == nullptr) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "内部错误："
                                                                "无法打开ffmpeg进程");
        std::abort();
    }

    int tm = 0;

    // start ffmpeg monitor thread
    std::thread([&]() {
        bool do_not_print_ffmpeg_info =
            config_.verbose_ & static_cast<int>(config::systemVerboseMaskEnum::NO_FFMPEG);

        const auto p1 = std::chrono::system_clock::now();
        std::string log_file_name = fmt::format(
            "live_render_{}.log",
            std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch())
                .count());

        auto log_fp = fopen(log_file_name.c_str(), "wb+");
        if (log_fp == nullptr) {
            fmt::print(fg(fmt::color::deep_sky_blue), "无法创建log文件\n");
        }

        std::string ffmpeg_monitor_str(1024, 0);
        while (true) {
            if (!fgets(ffmpeg_monitor_str.data(), 1023, p_stderr) || ferror(p_stderr) ||
                feof(p_stderr)) {
                break;
            }

            auto it_time = ffmpeg_monitor_str.find("time=");
            auto it_speed = ffmpeg_monitor_str.find("speed=");
            if (it_time != std::string::npos && it_speed != std::string::npos) {
                // time=00:00:19.32
                int _hour, _mins, _secs, _hundredths;
                sscanf(ffmpeg_monitor_str.data() + it_time + 5, "%d:%d:%d.%d", &_hour,
                       &_mins, &_secs, &_hundredths);

                kFfmpeg_output_time = (60 * 60 * 1000) * _hour + (60 * 1000) * _mins +
                                      (1000) * _secs + (10) * _hundredths;

                this->live_monitor_handle_->update_ffmpeg_time(kFfmpeg_output_time);
            }

            if (!do_not_print_ffmpeg_info) {
                printf("%s\n", ffmpeg_monitor_str.data());
            }

            if (log_fp) {
                fwrite(ffmpeg_monitor_str.c_str(), strlen(ffmpeg_monitor_str.c_str()), 1,
                       log_fp);
                fflush(log_fp);
            }
        }
        // TODO: post task handle

        if (log_fp) {
            fflush(log_fp);
            fclose(log_fp);
        }

        exit(0);
    }).detach();

    image_t *frame = &(this->ass_img_);
    //ass_set_cache_limits(ass_renderer, 0, 50); // save memory

    const size_t buffer_count = config_.video_height_ * config_.video_width_ * 4;

    danmaku::DanmakuHandle handle;
    handle.init_danmaku_screen_dialogue(this->config_);

    wait_queue_ready(this->danmaku_queue_);

    // overlay fully transparent img on first frame
    auto sz = fwrite(frame->buffer, 1, buffer_count, ffmpeg_);

    fmt::print(fg(fmt::color::green_yellow), "\n录制中...\n");

    // TODO: stop cond
    volatile bool stop_cond = true;

    bool wait_render = false;
    int wait_render_count = 0;
    int wait_render_offset_time = -1;

    const int step = ((double)(1000) / (double)(config_.fps_));

    ASS_Image *img;
    while (stop_cond) {
        using namespace std::chrono_literals;

        if (tm - kFfmpeg_output_time > 3000) {
            // render too fast. just slow down.
            std::this_thread::sleep_for(100ms);
        }

        if (kFfmpeg_output_time - tm > 5000) {
            wait_render_count += 2;
        } else if (kFfmpeg_output_time - tm > 3000) {
            wait_render_count += 1;
        } else if (!wait_render) {
            update_libass_event(ass_track, handle, this->config_, tm, false,
                                this->danmaku_queue_, this->live_monitor_handle_);
            wait_render_count = (std::max)(0, wait_render_count - 1);
        }

        if (wait_render_count > ((config_.fps_ / 5) * 6) && !wait_render) { // wait 6s
            wait_render = true;
        }

        if (wait_render) {
            if (wait_render_offset_time == -1) {
                // set
                float sec = handle.get_max_danmaku_end_time(config_.danmaku_move_time_,
                                                            config_.danmaku_pos_time_) +
                            0.1f;
                wait_render_offset_time = sec * 1000.0f; // sec to ms

                //fmt::print(fg(fmt::color::deep_sky_blue),
                //           "\ntoo slow, now tm{}, ffmpeg{}, wait{}\n", tm,
                //           kFfmpeg_output_time, wait_render_offset_time);
            } else if (tm > wait_render_offset_time) {
                // wait done.
                fmt::print(fg(fmt::color::deep_sky_blue), "\n弹幕渲染较慢，调整中...\n");

                if (kFfmpeg_output_time > wait_render_offset_time) {
                    tm = kFfmpeg_output_time + 1000; // FIXME: real time ffmpeg!
                } else {
                    tm = wait_render_offset_time + 3000;
                }

                // lagging. update danmaku now
                update_libass_event(ass_track, handle, this->config_, tm, true,
                                    this->danmaku_queue_, this->live_monitor_handle_);

                wait_render = false;
                wait_render_count = 0;
                wait_render_offset_time = -1;
            }
        }

        // clear buffer
        memset(frame->buffer, 0, buffer_count * 5);

        img = ass_render_frame(ass_renderer, ass_track, tm, NULL);
        blend(frame, img, 0);
        tm += step;

        img = ass_render_frame(ass_renderer, ass_track, tm, NULL);
        blend(frame, img, buffer_count * 1);
        tm += step;

        img = ass_render_frame(ass_renderer, ass_track, tm, NULL);
        blend(frame, img, buffer_count * 2);
        tm += step;

        img = ass_render_frame(ass_renderer, ass_track, tm, NULL);
        blend(frame, img, buffer_count * 3);
        tm += step;

        img = ass_render_frame(ass_renderer, ass_track, tm, NULL);
        blend(frame, img, buffer_count * 4);
        tm += step;

        auto sz = fwrite(frame->buffer, 5, buffer_count, ffmpeg_);

        this->live_monitor_handle_->update_ass_render_time(tm);
    }
}

void ffmpeg_render::main_thread() {
    std::thread([this]() { this->run(); }).detach();
}
