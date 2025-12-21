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
#include "ass_render.h"
#include "ass_render_utils.h"
#include "sc_control.h"
#include "danmaku_handle.h"
#include "ffmpeg_render.h"
#include "ffmpeg_utils.h"
#include "git.h"
#include "live_monitor.h"

#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/subprocess/subprocess.h"

extern bool kIs_ffmpeg_started;

inline int kDanmaku_inserted_count = 0;
inline volatile int kFfmpeg_output_time = 0; // time in ms
inline volatile uint64_t kReal_world_time_base = 0;
inline volatile uint64_t kReal_world_time = 0;

std::string kTest_sc_string;

extern "C" {
int ass_process_events_line(ASS_Track *track, char *str);
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

inline void wait_ffmpeg_ready(bool &is_ffmpeg_ready) {
    using namespace std::chrono_literals;

    while (!is_ffmpeg_ready) {
        std::this_thread::sleep_for(10ms);
    }

    kIs_ffmpeg_started = true;
}

inline void update_danmaku_event(
    ass::danmaku_ass_render_control *ass_object, danmaku::DanmakuHandle &handle,
    config::ass_config_t &config, int base_time, bool is_base_time_lag,
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
            if ((is_base_time_lag && 1) || // base_time - min_start_time < 6 * 1000
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
                ass_process_events_line(ass_object->get_track(), event_str.data());
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

    std::string sc_ass_header_str;
    if (config_.sc_enable_) {
        config::live_render_config_t copy_cfg = config_;
        copy_cfg.font_size_ = copy_cfg.sc_font_size_;
        copy_cfg.font_alpha_ = copy_cfg.sc_alpha_;

        sc_ass_header_str = ass::get_sc_ass_header(copy_cfg, ass_dialogue_list);
    } else {
        sc_ass_header_str = ass::get_sc_ass_header(config_, ass_dialogue_list);
    }

    kTest_sc_string = sc_ass_header_str;

    //    ASS_Track *ass_track =
    //        ass_read_memory(ass_library, const_cast<char *>(ass_header_str.c_str()),
    //                        ass_header_str.size(), NULL);

    ass::danmaku_ass_render_control danmaku_render{ass_library};
    ass::sc_ass_render_control sc_render{ass_library};

    danmaku_render.create_track(const_cast<char *>(ass_header_str.c_str()),
                                ass_header_str.size());
    sc_render.create_track(const_cast<char *>(sc_ass_header_str.c_str()),
                           sc_ass_header_str.size());
    // TODO: free tracker
    auto sc_tracker =
        ass_read_memory(ass_library, const_cast<char *>(sc_ass_header_str.c_str()),
                        sc_ass_header_str.size(), NULL);
    ass::TextProcess::Init(ass_library, ass_renderer, sc_tracker);

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

    bool is_ffmpeg_started = false;

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
        } else {
            // record version info
            std::string version_str =
                fmt::format("live_render version {}\n", GitMetadata::Describe());
            fwrite(version_str.c_str(), strlen(version_str.c_str()), 1, log_fp);
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

                is_ffmpeg_started = true;
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

        this->live_monitor_handle_->exit_live_render();
    }).detach();

    image_t *frame = &(this->ass_img_);
    //ass_set_cache_limits(ass_renderer, 0, 50); // save memory

    const size_t buffer_count = config_.video_height_ * config_.video_width_ * 4;

    danmaku::DanmakuHandle handle;
    handle.init_danmaku_screen_dialogue(this->config_);

    memset(frame->buffer, 0, buffer_count); // clear buffer
    // overlay fully transparent img on first second to indicate that this pipe has been started.
    for (int i = 0; i < this->config_.fps_; i++) {
        auto sz = fwrite(frame->buffer, 1, buffer_count, ffmpeg_);
    }

    // time setting.

    wait_ffmpeg_ready(is_ffmpeg_started);
    kReal_world_time_base = get_now_timestamp();

    assert(this->live_monitor_handle_ != nullptr);
    assert(this->live_danmaku_handle_ != nullptr);
    assert(this->config_.danmaku_lead_time_compensation_ <= 0);

    this->live_danmaku_handle_->update_base_time(
        kReal_world_time_base - 1000 + this->config_.danmaku_lead_time_compensation_);

    this->live_monitor_handle_->update_real_world_time_base(kReal_world_time_base);

    //wait_queue_ready(this->danmaku_queue_);

    fmt::print(fg(fmt::color::green_yellow), "\n录制中...\n");

    // TODO: stop cond
    volatile bool stop_cond = true;

    bool wait_render = false;
    bool print_flag = false;
    int wait_render_count = 0;
    int wait_render_offset_time = -1;

    double fast_step = ((double)(1000) / (double)(config_.fps_)) * (double)(1.5);
    double raw_step = ((double)(1000) / (double)(config_.fps_));
    double step = ((double)(1000) / (double)(config_.fps_));
    double double_tm = 0;
    double sc_double_tm = 0;
    double sc_double_end_update_tm = 0;
    int sc_tm = 0;

    ASS_Image *img;
    sc::ScControl sc_control{&sc_render, config_};
    while (stop_cond) {
        using namespace std::chrono_literals;

        if (this->config_.sc_enable_) {
            int sc_end_time;

            for (int i = 0; i < 5; i++) {
                sc_double_end_update_tm += step;
            }
            sc_end_time = static_cast<int>(sc_double_end_update_tm);

            sc_control.updateSuperChatEvent(&sc_render, this->config_, sc_tm,
                                            sc_double_end_update_tm - sc_tm,
                                            this->sc_queue_, this->live_monitor_handle_);
        }

        update_danmaku_event(&danmaku_render, handle, this->config_, tm, false,
                             this->danmaku_queue_, this->live_monitor_handle_);

        // render too fast. just slow down
        if (kReal_world_time - tm < 6 * 1000) {
            // As a rule of thumb, danmaku are usually pushed with a 5s delay.
            // We need to wait for this duration.
            std::this_thread::sleep_for(1000ms);
        } else if (kReal_world_time - tm < 10 * 1000) {
            // For a 1s rendering time, set the delay to 1s of the actual time.

            // actual time = 1s / (actual_fps)
            // actual_fps = config.fps / 5  ---> Now, we process 5 images at a time.
            double delay_time = 1000.0 * 5.0 / (double)config_.fps_;
            std::chrono::duration<double, std::milli> delay_count{delay_time};
            std::this_thread::sleep_for(delay_count);
        }

        // There should not be a slower situation, unless the machine's performance is poor
        // or the load of the danmaku is just too much.
        if (kFfmpeg_output_time - tm > 5 * 1000) {
            wait_render_count += 1;
        } else if (!wait_render) {
            wait_render_count = (std::max)(0, wait_render_count - 1);
        }

        if (wait_render_count > ((config_.fps_ / 5) * 6) && !wait_render) { // wait 6s
            wait_render = true;
        }

        if (wait_render) {
            step = fast_step;
            if (!print_flag) {
                print_flag = true;
                fmt::print(fg(fmt::color::deep_sky_blue), "\n弹幕渲染较慢，调整中...\n");
            }
            if (kFfmpeg_output_time - tm < 1 * 1000) {
                fmt::print(fg(fmt::color::deep_sky_blue), "\n调整完毕\n");
                step = raw_step;
                print_flag = false;
                wait_render = false;
                wait_render_count = 0;
            }
        }

        //if (wait_render) {
        //    if (wait_render_offset_time == -1) {
        //        // set
        //        float sec = handle.get_max_danmaku_end_time(config_.danmaku_move_time_,
        //                                                    config_.danmaku_pos_time_) +
        //                    0.1f;
        //        wait_render_offset_time = sec * 1000.0f; // sec to ms

        //        //fmt::print(fg(fmt::color::deep_sky_blue),
        //        //           "\ntoo slow, now tm{}, ffmpeg{}, wait{}\n", tm,
        //        //           kFfmpeg_output_time, wait_render_offset_time);
        //    } else if (tm > wait_render_offset_time) {
        //        // wait done.
        //        fmt::print(fg(fmt::color::deep_sky_blue), "\n弹幕渲染较慢，调整中...\n");

        //        if (kFfmpeg_output_time > wait_render_offset_time) {
        //            tm = kFfmpeg_output_time + 1000; // FIXME: real time ffmpeg!
        //        } else {
        //            tm = wait_render_offset_time + 3000;
        //        }

        //        // lagging. update danmaku now
        //        update_danmaku_event(ass_track, handle, this->config_, tm, true,
        //                            this->danmaku_queue_, this->live_monitor_handle_);

        //        wait_render = false;
        //        wait_render_count = 0;
        //        wait_render_offset_time = -1;
        //    }
        //}

        // clear buffer
        memset(frame->buffer, 0, buffer_count * 5);


        img = ass_render_frame(ass_renderer, danmaku_render.get_track(), tm, NULL);
        ass::ass_blend(frame, img, 0);
        double_tm += step;
        tm = static_cast<int>(double_tm);

        img = ass_render_frame(ass_renderer, danmaku_render.get_track(), tm, NULL);
        ass::ass_blend(frame, img, buffer_count * 1);
        double_tm += step;
        tm = static_cast<int>(double_tm);

        img = ass_render_frame(ass_renderer, danmaku_render.get_track(), tm, NULL);
        ass::ass_blend(frame, img, buffer_count * 2);
        double_tm += step;
        tm = static_cast<int>(double_tm);

        img = ass_render_frame(ass_renderer, danmaku_render.get_track(), tm, NULL);
        ass::ass_blend(frame, img, buffer_count * 3);
        double_tm += step;
        tm = static_cast<int>(double_tm);

        img = ass_render_frame(ass_renderer, danmaku_render.get_track(), tm, NULL);
        ass::ass_blend(frame, img, buffer_count * 4);
        double_tm += step;
        tm = static_cast<int>(double_tm);

        // sc layer is above danmaku layer
        if (config_.sc_enable_) [[unlikely]] {
            img = ass_render_frame(ass_renderer, sc_render.get_track(), sc_tm, NULL);
            ass::ass_blend(frame, img, 0);
            sc_double_tm += step;
            sc_tm = static_cast<int>(sc_double_tm);

            img = ass_render_frame(ass_renderer, sc_render.get_track(), sc_tm, NULL);
            ass::ass_blend(frame, img, buffer_count * 1);
            sc_double_tm += step;
            sc_tm = static_cast<int>(sc_double_tm);

            img = ass_render_frame(ass_renderer, sc_render.get_track(), sc_tm, NULL);
            ass::ass_blend(frame, img, buffer_count * 2);
            sc_double_tm += step;
            sc_tm = static_cast<int>(sc_double_tm);

            img = ass_render_frame(ass_renderer, sc_render.get_track(), sc_tm, NULL);
            ass::ass_blend(frame, img, buffer_count * 3);
            sc_double_tm += step;
            sc_tm = static_cast<int>(sc_double_tm);

            img = ass_render_frame(ass_renderer, sc_render.get_track(), sc_tm, NULL);
            ass::ass_blend(frame, img, buffer_count * 4);
            sc_double_tm += step;
            sc_tm = static_cast<int>(sc_double_tm);
        }

        auto sz = fwrite(frame->buffer, 5, buffer_count, ffmpeg_);

        kReal_world_time = get_now_timestamp() - kReal_world_time_base;
        this->live_monitor_handle_->update_real_world_time(
            static_cast<int>(kReal_world_time));
        this->live_monitor_handle_->update_ass_render_time(tm);
    }
}

void ffmpeg_render::main_thread() {
    std::thread([this]() { this->run(); }).detach();
}
