﻿#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
#endif

#include <cstdio>
#include <filesystem>
#include <vector>

#include "ffmpeg_utils.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

#include "thirdparty/re2/re2/re2.h"

#if defined(_WIN32) || defined(_WIN64)
#include "windows.h"

// TODO: remove this?
#define POPEN  _popen
#define PCLOSE _pclose
const char *kOpenOption = "wb";
const char *kOpenReadOption = "rb";
#else
#define POPEN  popen
#define PCLOSE pclose
const char *kOpenOption = "w";
const char *kOpenReadOption = "r";
#endif

/**
 * Check if the given path is correct. If the output path does not exist, an attempt will be made to create it.
 * If there are any errors, the program will terminate.
 *
 * @param config
 */
void check_live_render_path(config::live_render_config_t &config) {

    using namespace std::filesystem;

    auto error_output = [](auto str) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "找不到{}，请检查\n",
                   str);
        std::abort();
    };

    // step1: check ffmpeg
    auto ffmpeg_path_str = config.ffmpeg_path_;
    auto ffmpeg_path = u8path(config.ffmpeg_path_);
    if (!exists(ffmpeg_path)) {
        error_output("ffmpeg");
    }

#if defined(_WIN32) || defined(_WIN64)
    ffmpeg_path_str = config.ffmpeg_path_ + std::string("/ffmpeg.exe");
#else
    ffmpeg_path_str = config.ffmpeg_path_ + std::string("/ffmpeg");
#endif

    std::error_code _ec;
    directory_entry ffmpeg_entry(u8path(ffmpeg_path_str), _ec);

    if (!exists(ffmpeg_entry)) {
        error_output("ffmpeg");
    }

    // step2: check output file location
    auto output_path_str = config.output_file_path_;
    auto output_path = u8path(output_path_str);
    if (exists(output_path)) {
        directory_entry output_entry(output_path, _ec);
        if (!output_entry.is_directory()) {
            error_output(fmt::format("视频输出文件夹:\"{}\"", config.output_file_path_));
        }
    } else {
        // try to create new directory

        std::filesystem::create_directories(output_path);
        // the current compiler implementation is wrong, this return value cannot be used.

        auto output_pat_new = u8path(output_path_str);
        if (!exists(output_pat_new)) {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                       "无法创建视频输出文件夹:{}\n", config.output_file_path_);

            std::abort();
        }

        fmt::print(fg(fmt::color::green_yellow), "视频输出文件夹不存在，自动创建:{}\n",
                   config.output_file_path_);
    }
}

inline bool is_valid_file_name(const std::string &filename) {

#if defined(_WIN32) || defined(_WIN64)

    // re2 doesn't support look ahead
    // A two-step process is taken here. Pay attention to judgment conditions.
    RE2 re_device_name(
        R"(^(COM[0-9]|CON|LPT[0-9]|NUL|PRN|AUX|com[0-9]|con|lpt[0-9]|nul|prn|aux|[\s\.])((\..*)$|$))");
    assert(re_device_name.ok());

    RE2 re_char(R"(\A[^\\\/:*"?<>|]{1,1000}\z)");
    assert(re_char.ok());

    if (RE2::PartialMatch(filename, re_device_name)) {
        return false;
    }

    if (RE2::PartialMatch(filename, re_char) == false) {
        return false;
    }

    return true;
#else
    RE2 re_posix(R"(\A[^/]*\z)");
    assert(re_posix.ok());

    if (RE2::PartialMatch(filename, re_posix)) {
        return true;
    }

    return false;
#endif
}

inline std::string get_random_file_name(config::live_render_config_t &config) {
    const auto p1 = std::chrono::system_clock::now();
    std::string filename;

    if (config.segment_time_ > 0) {
        filename = fmt::format(
            "live_uid_{}_time_{}_part%03d_raw.mp4", config.user_uid_,
            std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch())
                .count());
    } else {
        filename = fmt::format(
            "live_uid_{}_time_{}_raw.mp4", config.user_uid_,
            std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch())
                .count());
    }

    config.actual_file_name_ = filename;

    return fmt::format("{}/{}", config.output_file_path_, filename);
}

// Dealing with the such minutiae is just annoying! And now you will see a bunch of nasty code.
// Using these win32 api is definitely a bad taste. Gosh, Windows also does a bunch of conversion operations
// inside the api implementation to enhance compatibility. Why isn't there a tiny cross-platform library?
// Why doesn't std::filesystem provide such a thing? Maybe std::filesystem itself doesn't handle character sets well. God knows!

// The user had better to make sure to provide a simple path that doesn't contain some weird stuff.
//

/**
 *  Gets the ffmpeg output file name from the configured file name and output path,
 *  and then set the actual file name im config parameter.
 *
 * @param config
 * @return
 */
inline std::string get_output_file_path(config::live_render_config_t &config) {

    // UTF8 filename
    const std::string raw_filename = config.filename_;

    // add the unix timestamp and check whether it is an appropriate file name
    const auto p1 = std::chrono::system_clock::now();
    std::string live_render_output_file_name;

    if (config.segment_time_ > 0) {
        live_render_output_file_name = fmt::format(
            "{}_{}_part%03d_raw.mp4", raw_filename,
            std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch())
                .count());
    } else {
        live_render_output_file_name = fmt::format(
            "{}_{}_raw.mp4", raw_filename,
            std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch())
                .count());
    }

    if (!is_valid_file_name(live_render_output_file_name)) {
        return get_random_file_name(config);
    }

    config.actual_file_name_ = live_render_output_file_name;

    std::string full_name =
        fmt::format("{}/{}", config.output_file_path_, live_render_output_file_name);

    return full_name;
}

inline std::string get_ffmpeg_file_path(const config::live_render_config_t &config) {
    std::string full_name = fmt::format("{}/ffmpeg", config.ffmpeg_path_);
    return full_name;
}

/**
 *
 * @param stream_address
 * @param config
 * @return Whether the information was successfully obtained
 */
inline bool ffmpeg_get_stream_meta_info(const std::string stream_address,
                                        config::live_render_config_t &config) {

    std::string buffer;
    buffer.resize(10240);

    std::string ffmpeg_exe_path = get_ffmpeg_file_path(config);

    std::vector<const char *> ffmpeg_cmd_line{
        ffmpeg_exe_path.c_str(),

        "-referer",
        "https://live.bilibili.com/",
        "-user_agent",
        "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/86.0.4240.198 Safari/537.36",

        "-i",
        stream_address.c_str(),
        nullptr,
    };

    subprocess_s subprocess;
    int ret = subprocess_create(ffmpeg_cmd_line.data(),
                                subprocess_option_inherit_environment, &subprocess);

    if (ret != 0) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "内部错误：无法执行ffmpeg\n");
        std::abort();
    }

    FILE *fp = subprocess_stderr(&subprocess);
    if (fp == NULL) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "内部错误：无法打开ffmpeg\n");
        std::abort();
    }

    int displayWidth = -1, displayHeight = -1, fps = -1;

    auto get_item = [&](auto index, int &item) {
        if (index != std::string::npos) {
            auto start_index = strstr(buffer.data() + index, ":");
            assert(start_index != nullptr);
            sscanf(start_index, ":%d", &item);
        }
    };

    /*
    * Stream #0:1: Video: h264 (High), yuv420p(tv, bt709, progressive), 1920x1080, 10240 kb/s, 60 fps, 60 tbr, 1k tbn
    */
    std::size_t pos;
    char *substr;

    while (!feof(fp)) {
        if (fgets(buffer.data(), 10240 - 1, fp) != NULL) {
            if (buffer.find("Stream") == std::string::npos) {
                continue;
            }

            if ((pos = buffer.find("Video")) == std::string::npos) {
                continue;
            }

            // get displayWidth and displayHeight
            // From ffmpeg "avcodec_string()"
            //   Stream #0:0: Video: h264 (High) (avc1 / 0x31637661), yuv420p(tv, bt709), 1280x720 [SAR 1:1 DAR 16:9], 0 kb/s, 30 fps, 30 tbr, 90k tbn (default)
            //   Stream #0:0: Video: h264 (High), yuv420p(tv, bt470bg/bt470bg/smpte170m, progressive), 1920x1080, 10240 kb/s, 60 fps, 60 tbr, 1k tbn
            //   Stream #0:1: Video: h264 (High) ([27][0][0][0] / 0x001B), yuv420p(tv, bt709), 1920x1080, 60 fps, 30 tbr, 90k tbn
            pos = buffer.find("fps");
            assert(pos != std::string::npos);

            pos = buffer.find_last_of("x", pos);
            assert(pos != std::string::npos);

            // , 1920x1080
            pos = buffer.find_last_of(",", pos);
            assert(pos != std::string::npos);
            //  1920x1080
            substr = buffer.data() + pos + 1;

            ret = sscanf(substr, "%dx%d", &displayWidth, &displayHeight);
            assert(ret == 2);

            // get fps
            pos = buffer.find("fps", pos);
            assert(pos != std::string::npos);
            pos = buffer.find_last_of(",", pos);
            substr = buffer.data() + pos + 1;

            sscanf(substr, "%d", &fps);

            break;
        }
    }

    subprocess_destroy(&subprocess);

    if (displayWidth == -1 || displayHeight == -1 || fps == -1) {
        return false;
    }

    config.stream_address_ = stream_address;

    config.origin_video_fps_ = fps;
    config.origin_video_height_ = displayHeight;
    config.origin_video_width_ = displayWidth;

    // when user not set video config, use origin video setting.
    config.video_width_ =
        config.adjust_video_width_ ? config.adjust_video_width_ : displayWidth;

    config.video_height_ =
        config.adjust_video_height_ ? config.adjust_video_height_ : displayHeight;

    config.fps_ = config.adjust_video_fps_ ? config.adjust_video_fps_ : fps;

    config.adjust_video_height_ =
        config.adjust_video_height_ ? config.adjust_video_height_ : config.video_height_;
    config.adjust_video_width_ =
        config.adjust_video_width_ ? config.adjust_video_width_ : config.video_width_;
    config.adjust_video_fps_ =
        config.adjust_video_fps_ ? config.adjust_video_fps_ : config.fps_;

    return true;
}

bool init_stream_video_info(const std::vector<live_stream_info_t> &stream_list,
                            config::live_render_config_t &config) {

    std::vector<live_stream_info_t> h264_stream_list;

    if (stream_list.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "无法获取直播流地址，重试\n");
        return false;
    }

    for (auto &item : stream_list) {
        if (item.protocol_ == live_stream_info_t::STREAM &&
            item.codec_ == live_stream_info_t::AVC &&
            item.format_ == live_stream_info_t::FLV) {
            h264_stream_list.push_back(item);
        }
    }

    if (h264_stream_list.empty()) {
        fmt::print(fg(fmt::color::orange) | fmt::emphasis::italic,
                   "无法获取FLV流，尝试添加其他流\n");
        for (auto &item : stream_list) {
            h264_stream_list.push_back(item);
        }
    }

    bool flag = false;
    for (auto &item : h264_stream_list) {
        config.live_stream_format_ = static_cast<int>(item.format_);
        if (ffmpeg_get_stream_meta_info(item.address_, config)) {
            flag = true;
            break;
        }
    }

    if (!flag) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "无法与直播流建立连接，重试\n");
    }

    return flag;
}

inline void process_input_video_filter(std::string &filter_str,
                                       const config::live_render_config_t &config) {
    bool has_prefix_filter = false;

    if (config.origin_video_width_ == config.adjust_video_width_ &&
        config.origin_video_height_ == config.adjust_video_height_ &&
        config.origin_video_fps_ == config.adjust_video_fps_) {
        filter_str = "[0:v]"; // do not change input video
        return;
    }

    if (config.origin_video_width_ < config.origin_video_height_ &&
        config.adjust_video_height_ < config.adjust_video_width_) {
        // convert a vertical video to horizontal
        // and add black padding.

        // padding = 1/2 * (new_video_width - scaled_video_width)
        double scaled_video_width =
            (double)config.origin_video_width_ *
            ((double)config.adjust_video_height_ / (double)config.origin_video_height_);
        double padding_left = 0.5 * (config.adjust_video_width_ - scaled_video_width);

        if (padding_left < 0) {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                       "\n原视频无法进行调整，原始尺寸为{}x{}，预期尺寸为{}x{}\n",
                       config.origin_video_width_, config.origin_video_height_,
                       config.adjust_video_width_, config.adjust_video_height_);
            std::abort();
        }

        filter_str = fmt::format("[0:v]scale=-1:{},pad={}:{}:{}:0",
                                 config.adjust_video_height_, config.adjust_video_width_,
                                 config.adjust_video_height_, (int)padding_left);

        has_prefix_filter = true;
    } else if (config.origin_video_height_ == config.adjust_video_height_ &&
               config.origin_video_width_ == config.adjust_video_width_) {
        // nothing to do
        has_prefix_filter = false;
    } else {
        // just scale input video
        filter_str = fmt::format("[0:v]scale={}:{}", config.adjust_video_width_,
                                 config.adjust_video_height_);

        has_prefix_filter = true;
    }

    // change fps
    if (config.origin_video_fps_ != config.adjust_video_fps_) {
        if (has_prefix_filter) {
            filter_str += ","; // spilt multiple filter
        } else {
            filter_str = "[0:v]"; // fps is the first filter
        }

        filter_str += fmt::format("fps={}", config.adjust_video_fps_);
        has_prefix_filter = true;
    }

    // output process [0:v] ===> [lr_v0]
    filter_str += "[lr_v0];[lr_v0]";
}

/**
 * Process ffmpeg command line and create a ffmpeg subprocess.
 * If there are any errors, the program will terminate.
 *
 * @param subprocess Create ffmpeg subprocess
 * @param config
 */
void init_ffmpeg_subprocess(struct subprocess_s *subprocess,
                            config::live_render_config_t &config) {
    using namespace fmt::literals;

    std::string ffmpeg_exe_path = get_ffmpeg_file_path(config);
    std::string output_file_path = get_output_file_path(config);

    std::string ffmpeg_video_info =
        fmt::format("{video_width}x{video_height}", "video_width"_a = config.video_width_,
                    "video_height"_a = config.video_height_);
    std::string ffmpeg_fps_info = fmt::format("{}", config.fps_);

    std::string ffmpeg_segment_time = fmt::format("{}", config.segment_time_);
    std::string ffmpeg_thread_queue_size =
        fmt::format("{}", config.ffmpeg_thread_queue_size_);
    std::string render_thread_queue_size =
        fmt::format("{}", config.render_thread_queue_size_);

    bool ffmpeg_copy_audio = std::string("copy") == config.audio_bitrate_;
    // cmd line

    // TODO: reconnect?

    // clang-format off
    std::vector<const char *> ffmpeg_cmd_line{
        ffmpeg_exe_path.c_str(),
        "-y",

        "-referer",
        "https://live.bilibili.com/",
        "-user_agent",
        "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.198 Safari/537.36",

        "-fflags",
        "+discardcorrupt",
        "-analyzeduration", "400000", // 400ms
    };

    if (config.live_stream_format_ == static_cast<int>(live_stream_info_t::FLV)) {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
                // forces ffmpeg to extract frames as-is instead of trying to match a framerate
                "-vsync", "passthrough",
            });
    }

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

    process_input_video_filter(ffmpeg_overlay_filter_str, config);
    // fmp4 process
    if (config.live_stream_format_ == static_cast<int>(live_stream_info_t::FMP4)) {
        ffmpeg_overlay_filter_str +=
            fmt::format("setpts=N/({}*TB)[lr_v0_fmp4];[lr_v0_fmp4]", ffmpeg_fps_info);
    }

    // FFmpeg improved the calculation of premultiplied alpha for YUV format.
    // Now we don't have to make extra pixel format conversion anymore.
    // See: https://git.ffmpeg.org/gitweb/ffmpeg.git/commit/bdf01a9

    if (0) {
        // quality first
        ffmpeg_overlay_filter_str +=
            "[1:v]overlay=x=0:y=0:alpha=premultiplied:format=rgb";
    } else {
        // default option (speed first)
        // For the newer ffmpeg, this actually performs better.
        ffmpeg_overlay_filter_str +=
            "[1:v]overlay=x=0:y=0:alpha=premultiplied:format=yuv420";
    }

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
            "-i",
            config.stream_address_.c_str(),
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
            //"[0:v][1:v]overlay=eof_action=endall[v]",
            "-map", "[v]",
            "-map", "0:a",
    });

    if (ffmpeg_copy_audio) {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-c:a", "copy",
        });
    }

    // set video encoder
    ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-c:v:0",
            config.encoder_.c_str(),
    });

    // old version FFmpeg alpha fix
    if (0) {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-pix_fmt", "nv12",
        });
    }

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

    if (!ffmpeg_copy_audio) {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
                "-b:a:0",
                config.audio_bitrate_.c_str(),
        });
    }

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

    if (config.live_stream_format_ != static_cast<int>(live_stream_info_t::FLV)) {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
                "-r",
                ffmpeg_fps_info.c_str(),
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
