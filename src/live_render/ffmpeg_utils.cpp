#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
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

// Refer :
// UTF-8 "support" in C++20 seems to be a bad joke.
//
// The only UTF functionality in the Standard Library is support for strings and
// string_views (std::u8string, std::u8string_view, std::u16string, ...). That is all. T
// here is no Standard Library support for UTF coding in regular expressions, formatting,
// file i/o and so on.
//
//
inline std::string str2local_codepage(const std::string str) {

#if defined(_WIN32) || defined(_WIN64)

    const auto sz = (str.size() + 1) * 6; // TODO: get actual length

    std::string utf16_cov;
    utf16_cov.resize(sz);

    std::string local_code_page_str;
    local_code_page_str.resize(sz);

    memset(utf16_cov.data(), 0, sz);
    memset(local_code_page_str.data(), 0, sz);

    // convert to UTF-16LE
    auto ret = simdutf::convert_utf8_to_utf16(
        str.data(), str.size(), reinterpret_cast<char16_t *>(utf16_cov.data()));

    if (ret == 0) {
        return "";
    }

    int ret2 = WideCharToMultiByte(
        CP_OEMCP, NULL, reinterpret_cast<LPCWCH>(utf16_cov.data()), -1,
        local_code_page_str.data(), local_code_page_str.size(), NULL, NULL);

    if (ret2 == 0) {
        // TODO: log output
        return "";
    }

    return local_code_page_str;

#else
    return str;
#endif
}

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
    auto ffmpeg_path_str = str2local_codepage(config.ffmpeg_path_);
    path ffmpeg_path(ffmpeg_path_str);
    if (!exists(ffmpeg_path)) {
        error_output("ffmpeg");
    }

#if defined(_WIN32) || defined(_WIN64)
    ffmpeg_path_str =
        str2local_codepage(config.ffmpeg_path_ + std::string("/ffmpeg.exe"));
#else
    ffmpeg_path_str = str2local_codepage(config.ffmpeg_path_ + std::string("/ffmpeg"));
#endif

    directory_entry ffmpeg_entry(ffmpeg_path_str);

    if (!exists(ffmpeg_entry)) {
        error_output("ffmpeg");
    }

    // step2: check output file location
    auto output_path_str = str2local_codepage(config.output_file_path_);
    path output_path(output_path_str);
    if (exists(output_path)) {
        directory_entry output_entry(output_path);
        if (!output_entry.is_directory()) {
            error_output(fmt::format("视频输出文件夹:\"{}\"", config.output_file_path_));
        }
    } else {
        // try to create new directory

        std::filesystem::create_directories(output_path);
        // the current compiler implementation is wrong, this return value cannot be used.

        path output_pat_new(output_path_str);
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

    if (RE2::PartialMatch(filename, re_device_name)) {
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

    return str2local_codepage(fmt::format("{}/{}", config.output_file_path_, filename));
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

#if defined(_WIN32) || defined(_WIN64)

    // covert to local codepage
    std::string cov_name = str2local_codepage(full_name);
    if (cov_name.empty()) {
        return get_random_file_name(config);
    }

    return cov_name;

#else
    return full_name;
#endif
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

    auto cmd = fmt::format("{}/ffmpeg -i \"{}\" 2>&1", config.ffmpeg_path_,
                           stream_address); // stderr > stdout

    FILE *fp = POPEN(cmd.c_str(), kOpenReadOption);
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

    while (!feof(fp)) {
        if (fgets(buffer.data(), 10240 - 1, fp) != NULL) {
            auto displayWidth_it = buffer.find("displayWidth");
            auto displayHeight_it = buffer.find("displayHeight");
            auto fps_it = buffer.find("fps");

            get_item(displayWidth_it, displayWidth);
            get_item(displayHeight_it, displayHeight);
            get_item(fps_it, fps);

            if (displayWidth != -1 && displayHeight != -1 && fps != -1) {
                break;
            }
        }
    }

    PCLOSE(fp);

    if (displayWidth == -1 && displayHeight == -1 && fps == -1) {
        return false;
    }

    config.stream_address_ = stream_address;

    if (displayWidth != -1) {
        config.video_width_ = displayWidth;
    }

    if (displayHeight != -1) {
        config.video_height_ = displayHeight;
    }

    if (fps != -1) {
        config.fps_ = fps;
    }

    return true;
}

void init_stream_video_info(const std::vector<live_stream_info_t> &stream_list,
                            config::live_render_config_t &config) {

    std::vector<live_stream_info_t> h264_stream_list;

    if (stream_list.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "无法获取直播地址\n");
        std::abort(); // retry?
    }

    for (auto &item : stream_list) {
        if (item.protocol_ == live_stream_info_t::STREAM &&
            item.codec_ == live_stream_info_t::AVC &&
            item.format_ == live_stream_info_t::FLV) {
            h264_stream_list.push_back(item);
        }
    }

    if (stream_list.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "无法获取FLV流\n");
        std::abort(); // TODO: retry?
    }

    bool flag = false;
    for (auto &item : h264_stream_list) {
        if (ffmpeg_get_stream_meta_info(item.address_, config)) {
            flag = true;
        }
    }

    if (!flag) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "无法与直播流建立连接\n");
        std::abort(); // TODO: retry
    }
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

    std::string ffmpeg_exe_path = config.ffmpeg_path_ + "/ffmpeg";
    std::string output_file_path = get_output_file_path(config);

    std::string ffmpeg_video_info =
        fmt::format("{video_width}x{video_height}", "video_width"_a = config.video_width_,
                    "video_height"_a = config.video_height_);
    std::string ffmpeg_fps_info = fmt::format("{}", config.fps_);

    std::string ffmpeg_segment_time = fmt::format("{}", config.segment_time_);
    std::string ffmpeg_thread_queue_size = fmt::format("{}", config.thread_queue_size_);
    // cmd line

    // TODO: reconnect?

    // clang-format off
    std::vector<const char *> ffmpeg_cmd_line{
        ffmpeg_exe_path.c_str(),
        "-y",
        "-headers",
        "Content-Type: application/x-www-form-urlencoded\r\nUser-Agent: Mozilla/5.0 "
        "(Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/87.0.4280.141 Safari/537.36\r\n",
        "-fflags",
        "+discardcorrupt",
        "-vsync",
        "0",
        "-analyzeduration",
        "60",
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

    ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-thread_queue_size",
            ffmpeg_thread_queue_size.c_str(),
            "-i",
            config.stream_address_.c_str(),
            "-f",
            "rawvideo",
            "-s",
            ffmpeg_video_info.c_str(),
            "-pix_fmt",
            "rgba",
            "-r",
            ffmpeg_fps_info.c_str(),
            "-i",
            "-",
            "-filter_complex",
            "[0:v][1:v]overlay=0:0[v]",
            "-map",
            "[v]",
            "-map",
            "0:a",
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
            "-b:a:0",
            config.audio_bitrate_.c_str(),
    });

    // set segment time
    if (config.segment_time_ > 0) {
        ffmpeg_cmd_line.insert(ffmpeg_cmd_line.end(),{
            "-f",
            "segment",
            "-segment_time",
            ffmpeg_segment_time.c_str(),
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
