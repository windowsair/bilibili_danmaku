#include <string>

#include "test_sc_common.h"
#include "live_render_config.h"
#include "ffmpeg_render.h"
#include "ass_danmaku.h"
#include "ass_render_utils.h"
#include "sc_control.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/subprocess/subprocess.h"
#include "thirdparty/libass/include/ass.h"


constexpr int VIDEO_HEIGHT = 1080;
constexpr int VIDEO_WIDTH = 1920;
constexpr int FPS = 60;

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

inline std::string get_ffmpeg_file_path(const config::live_render_config_t &config) {
    std::string full_name = fmt::format("{}/ffmpeg", config.ffmpeg_path_);
    return full_name;
}

inline void init_sc_subprocess(struct subprocess_s *subprocess,
                               config::live_render_config_t &config) {
    using namespace fmt::literals;

    std::string ffmpeg_exe_path = get_ffmpeg_file_path(config);
    std::string output_file_path = "./sc_list/test_sc_control_output.mp4";

    std::string ffmpeg_video_info =
        fmt::format("{video_width}x{video_height}", "video_width"_a = config.video_width_,
                    "video_height"_a = config.video_height_);
    std::string ffmpeg_fps_info = fmt::format("{}", config.fps_);

    std::string ffmpeg_segment_time = fmt::format("{}", config.segment_time_);
    std::string ffmpeg_thread_queue_size =
        fmt::format("{}", config.ffmpeg_thread_queue_size_);
    std::string render_thread_queue_size =
        fmt::format("{}", config.render_thread_queue_size_);


    // clang-format off
    std::vector<const char *> ffmpeg_cmd_line{
        ffmpeg_exe_path.c_str(),
        "-y",
        "-fflags",
        "+discardcorrupt",
        "-vsync", "passthrough", // forces ffmpeg to extract frames as-is instead of trying to match a framerate
        "-analyzeduration", "60",
    };


    // add hardware decoder
    if (config.decoder_ != "none") {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
                "-hwaccel",
                config.decoder_.c_str(),
                //"-hwaccel_device",
                //"0",
            });
    }
    // clang-format on
    std::string ffmpeg_overlay_filter_str;
    ffmpeg_overlay_filter_str = "[0:v]format=nv12";

    // extra user filter process
    if (config.extra_filter_info_.empty()) {
        ffmpeg_overlay_filter_str += "[v]";
    } else {
        ffmpeg_overlay_filter_str += "[v0];" + config.extra_filter_info_;
    }

    // extra user input video parameter process
    auto split_string_space = [](const std::string &s,
                                 std::vector<std::string> &split_res) {
        std::string delim = " ";

        auto start = 0U;
        auto end = s.find(delim);

        while (end != std::string::npos) {
            std::string ss = s.substr(start, end - start);
            if (!ss.empty()) {
                split_res.push_back(ss);
            }

            start = end + delim.length();
            end = s.find(delim, start);
        }

        std::string ss = s.substr(start, end);
        if (!ss.empty()) {
            split_res.push_back(ss);
        }
    };

    std::vector<std::string> extra_input_stream_parameter_list;

    split_string_space(config.extra_input_stream_info_,
                       extra_input_stream_parameter_list);

    // clang-format off
    ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-thread_queue_size",
            ffmpeg_thread_queue_size.c_str(),
//            "-i",
//            config.stream_address_.c_str(),
            "-thread_queue_size",
            render_thread_queue_size.c_str(),
            "-f",
            "rawvideo",
            "-s",
            ffmpeg_video_info.c_str(),
            "-pix_fmt",
            "rgba",
            "-r",
            //"-framerate",
            ffmpeg_fps_info.c_str(),
            "-i",
            "-",
    });

    // extra video input parameter
    for (auto &item : extra_input_stream_parameter_list) {
        ffmpeg_cmd_line.push_back(item.c_str());
    }


    ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-filter_complex",
            ffmpeg_overlay_filter_str.c_str(),
            "-map", "[v]",
    });

    // set video encoder
    ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-c:v:0",
            config.encoder_.c_str(),
    });

    // clang-format on

    // add extra encoder info
    if (!config.extra_encoder_info_.empty()) {
        for (auto &item : config.extra_encoder_info_) {
            ffmpeg_cmd_line.push_back(item.c_str());
        }
    }

    // clang-format off

    // set bitrate
    ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-b:v:0",
            config.video_bitrate_.c_str(),
    });


    // set segment time
    if (config.segment_time_ > 0) {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-f",
            "segment",
            "-segment_time",
            ffmpeg_segment_time.c_str(),
            "-segment_format_options",
            "movflags=+frag_keyframe+empty_moov",
        });
    }
    else {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-f",
            "mp4",
        });
    }

    ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-movflags",
            "frag_keyframe+empty_moov", // This potentially increase disk IO usage.
                                        // However, the integrity of the video can be guaranteed in case of errors.
            output_file_path.c_str(),
            nullptr,
    });

    ffmpeg_cmd_line.push_back(nullptr);

    // clang-format on

    auto print_ffmpeg_cmd = [&]() {
        fmt::print("\n原始ffmpeg命令:\n");
        for (auto &item : ffmpeg_cmd_line) {
            if (item) {
                printf("%s ", item);
            }
        }
        fmt::print("\n");
    };

    // create ffmpeg subprocess
    print_ffmpeg_cmd();

    int ret = subprocess_create(ffmpeg_cmd_line.data(),
                                subprocess_option_inherit_environment, subprocess);
    if (ret != 0) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "内部错误：无法执行ffmpeg\n");
        std::abort();
    }
}

void main_worker(struct subprocess_s *subprocess, config::live_render_config_t &config,
                 moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> &sc_queue,
                 int max_alive_time) {
    ASS_Library *ass_library = nullptr;
    ASS_Renderer *ass_renderer = nullptr;
    std::vector<danmaku::ass_dialogue_t> ass_dialogue_list;
    std::string sc_ass_header_str;

    libass_init(&ass_library, &ass_renderer, config.video_width_, config.video_height_,
                true);

    config::live_render_config_t copy_cfg = config;
    copy_cfg.font_size_ = copy_cfg.sc_font_size_;
    copy_cfg.font_alpha_ = copy_cfg.sc_alpha_;

    sc_ass_header_str = ass::get_sc_ass_header(copy_cfg, ass_dialogue_list);
    ass::sc_ass_render_control sc_render{ass_library};

    sc_render.create_track(const_cast<char *>(sc_ass_header_str.c_str()),
                           sc_ass_header_str.size());

    FILE *ffmpeg_ = subprocess_stdin(subprocess);
    FILE *p_stderr = subprocess_stderr(subprocess);
    if (ffmpeg_ == nullptr || p_stderr == nullptr) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "内部错误："
                                                                "无法打开ffmpeg进程");
        std::abort();
    }

    // start ffmpeg monitor thread
    std::thread([&]() {
        std::string ffmpeg_monitor_str(1024, 0);
        while (true) {
            if (!fgets(ffmpeg_monitor_str.data(), 1023, p_stderr) || ferror(p_stderr) ||
                feof(p_stderr)) {
                break;
            }
            printf("%s\n", ffmpeg_monitor_str.data());
        }
        // TODO:exit thread
        //this->live_monitor_handle_->exit_live_render();
    }).detach();

    image_t frame;
    frame.width = config.video_width_;
    frame.height = config.video_height_;
    frame.stride = config.video_width_ * 4;
    frame.buffer = static_cast<unsigned char *>(malloc(frame.height * frame.stride * 5));

    const int buffer_count = frame.height * frame.stride;
    memset(frame.buffer, 0, buffer_count); // clear buffer
    // overlay fully transparent img on first second to indicate that this pipe has been started.
    for (int i = 0; i < config.fps_; i++) {
        auto sz = fwrite(frame.buffer, 1, buffer_count, ffmpeg_);
    }

    fmt::print(fg(fmt::color::green_yellow), "\nStart render...\n");

    volatile bool stop_cond = true;
    int tm = 0;
    double fast_step = ((double)(1000) / (double)(config.fps_)) * (double)(1.5);
    double raw_step = ((double)(1000) / (double)(config.fps_));
    double step = ((double)(1000) / (double)(config.fps_));
    double double_tm = 0;
    double double_sc_tm = 0;
    double double_sc_update_tm = 0;

    auto tmp_double_tm = double_tm;
    auto tmp_tm = tm;

    ASS_Image *img;
    sc::ScControl sc_control{&sc_render, config};

    while (stop_cond) {
        {
            int sc_end_time;

            for (int i = 0; i < 5; i++) {
                double_sc_update_tm += step;
            }
            sc_end_time = static_cast<int>(double_sc_update_tm);

            sc_control.updateSuperChatEvent(&sc_render, config, tmp_tm,
                                            sc_end_time - tmp_tm, &sc_queue, nullptr);
        }

        // clear buffer
        memset(frame.buffer, 0, buffer_count * 5);


        img = ass_render_frame(ass_renderer, sc_render.get_track(), tmp_tm, NULL);
        blend(&frame, img, 0);
        tmp_double_tm += step;
        tmp_tm = static_cast<int>(tmp_double_tm);

        img = ass_render_frame(ass_renderer, sc_render.get_track(), tmp_tm, NULL);
        blend(&frame, img, buffer_count * 1);
        tmp_double_tm += step;
        tmp_tm = static_cast<int>(tmp_double_tm);

        img = ass_render_frame(ass_renderer, sc_render.get_track(), tmp_tm, NULL);
        blend(&frame, img, buffer_count * 2);
        tmp_double_tm += step;
        tmp_tm = static_cast<int>(tmp_double_tm);

        img = ass_render_frame(ass_renderer, sc_render.get_track(), tmp_tm, NULL);
        blend(&frame, img, buffer_count * 3);
        tmp_double_tm += step;
        tmp_tm = static_cast<int>(tmp_double_tm);

        img = ass_render_frame(ass_renderer, sc_render.get_track(), tmp_tm, NULL);
        blend(&frame, img, buffer_count * 4);
        tmp_double_tm += step;
        tmp_tm = static_cast<int>(tmp_double_tm);

        auto sz = fwrite(frame.buffer, 5, buffer_count, ffmpeg_);
    }
}

int main() {
    std::string xml_input = "./sc_list/4.xml";
    moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> sc_queue;
    int max_alive_time = 0;

    max_alive_time = process_sc_list_from_file(xml_input, sc_queue);
    assert(max_alive_time != 0);

    struct subprocess_s subprocess;
    auto config = config::get_user_live_render_config();
    config.video_height_ = VIDEO_HEIGHT;
    config.video_width_ = VIDEO_WIDTH;
    config.fps_ = FPS;
    init_sc_subprocess(&subprocess, config);

    main_worker(&subprocess, config, sc_queue, max_alive_time);
    return 0;
}
