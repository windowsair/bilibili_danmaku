#include <algorithm>
#include <string>
#include <thread>

#include "ass_danmaku.h"
#include "ffmpeg_render.h"

#include "thirdparty/libass/include/ass.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

int all_count = 0;

extern "C" {
int ass_process_events_line(ASS_Track *track, char *str);
}

#if defined(_WIN32) || defined(_WIN64)
#define POPEN  _popen
#define PCLOSE _pclose
const char *kOpenOption = "wb";
#else
#define POPEN  popen
#define PCLOSE pclose
const char *kOpenOption = "w";
#endif

#define TO_R(c) ((c) >> 24)
#define TO_G(c) (((c) >> 16) & 0xFF)
#define TO_B(c) (((c) >> 8) & 0xFF)
#define TO_A(c) ((c)&0xFF)

inline void blend_single(image_t *frame, ASS_Image *img) {
    int x, y;
    unsigned char opacity = 255 - TO_A(img->color);
    unsigned char r = TO_R(img->color);
    unsigned char g = TO_G(img->color);
    unsigned char b = TO_B(img->color);

    unsigned char *src;
    unsigned char *dst;

    src = img->bitmap;
    dst = frame->buffer + img->dst_y * frame->stride + img->dst_x * 4;
    for (y = 0; y < img->h; ++y) {
        for (x = 0; x < img->w; ++x) {
            unsigned k = ((unsigned)src[x]) * opacity;
            // possible endianness problems
            dst[x * 4] = (k * r + (255 * 255 - k) * dst[x * 4]) / (255 * 255);
            dst[x * 4 + 1] = (k * g + (255 * 255 - k) * dst[x * 4 + 1]) / (255 * 255);
            dst[x * 4 + 2] = (k * b + (255 * 255 - k) * dst[x * 4 + 2]) / (255 * 255);
            dst[x * 4 + 3] = (k * 255 + (255 * 255 - k) * dst[x * 4 + 3]) / (255 * 255);
        }
        src += img->stride;
        dst += frame->stride;
    }
}

inline void blend(image_t *frame, ASS_Image *img) {
    int cnt = 0;
    while (img) {
        blend_single(frame, img);
        ++cnt;
        img = img->next;
    }
    //printf("%d images blended\n", cnt);
}

void libass_msg_callback(int level, const char *fmt, va_list va, void *data) {
    if (level > 6)
        return;
    printf("libass: ");
    vprintf(fmt, va);
    printf("\n");
}

inline void libass_init(ASS_Library **ass_library, ASS_Renderer **ass_renderer,
                        int frame_w, int frame_h) {
    *ass_library = ass_library_init();
    if (!(*ass_library)) {
        printf("ass_library_init failed!\n");
        std::abort();
    }

    ass_set_message_cb(*ass_library, libass_msg_callback, NULL);
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
    while (p == nullptr) {
        std::this_thread::sleep_for(10ms);
        p = queue->peek();
    }
}

inline void update_libass_event(
    ASS_Track *ass_track, danmaku::DanmakuHandle &handle, config::ass_config_t &config,
    int base_time,
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *queue) {

    // TODO:
    // We should ensure that the list is sorted in ascending chronological order.

    while (auto p = queue->peek()) {
        std::vector<danmaku::danmaku_item_t> &danmaku_list = *p;
        assert(!danmaku_list.empty());

        // get minimum start time
        float min_start_time_ =
            (std::min_element(
                 danmaku_list.begin(), danmaku_list.end(),
                 [](const danmaku::danmaku_item_t &a, const danmaku::danmaku_item_t &b) {
                     return a.start_time_ < b.start_time_;
                 }))
                ->start_time_;

        int min_start_time = min_start_time_ * 1000;

        bool render_danmaku = false;
        if (min_start_time < base_time) {
            if (base_time - min_start_time < 1000) { // distance 1s
                // The danmaku appear behind the current rendering time.
                // If the time difference is relatively short, then we pull the base time
                // of these danmaku after the current base time and render them.
                int offset_time_ = base_time - min_start_time + 10;
                float offset_time = offset_time_ / 1000.0f;
                for (auto &item : danmaku_list) {
                    item.start_time_ += offset_time;
                }
                render_danmaku = true;
                // TODO: move base time?
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

            // TODO: pos type?
            handle.process_danmaku_dialogue_move(danmaku_list, config, ass_dialogue_list);
            for (auto &item : ass_dialogue_list) {
                // convert to event_str
                std::string event_str = ass::get_ass_event(config, item);
                event_str.push_back('\0');
                // insert danmaku
                ass_process_events_line(ass_track, event_str.data());
            }

            all_count += ass_dialogue_list.size();
            printf("[%d]\n", all_count);
        }

        queue->pop();
    }
}

void ffmpeg_render::run() {
    using namespace fmt::literals;

    if (this->danmaku_queue_ == nullptr) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "内部错误：未设置弹幕队列\n");
        std::abort();
    }

    if (this->ffmpeg_input_address_.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "内部错误：未设置直播流地址\n");
        std::abort();
    }

    ASS_Library *ass_library = nullptr;
    ASS_Renderer *ass_renderer = nullptr;

    libass_init(&ass_library, &ass_renderer, config_.video_width_, config_.video_height_);

    std::vector<danmaku::ass_dialogue_t> ass_dialogue_list;

    std::string ass_header_str = ass::get_ass_header(config_, ass_dialogue_list);
    printf("%s", ass_header_str.c_str());

    ASS_Track *ass_track =
        ass_read_memory(ass_library, const_cast<char *>(ass_header_str.c_str()),
                        ass_header_str.size(), NULL);

    int tm = 0;

    // TODO: is ffmpeg exist?
    std::string ffmpeg_cmd = fmt::format(
        "K:/ff/ffmpeg.exe -y  -vsync 0 -hwaccel nvdec  " // -hwaccel_device 0
        "-i \"{input_address}\" -f rawvideo -s {video_width}x{video_height}"
        " -pix_fmt rgba -r 60 -i - "
        "-filter_complex [0:v][1:v]overlay=0:0[v] -map \"[v]\"  -map  \"0:a\"   "
        "-c:v:0 h264_nvenc -b:v:0 5650k -f mp4 \"K:/ff/my_test1.mp4\"",

        "input_address"_a = this->ffmpeg_input_address_,
        "video_width"_a = this->config_.video_width_,
        "video_height"_a = this->config_.video_height_);

    FILE *ffmpeg_ = POPEN(ffmpeg_cmd.c_str(), kOpenOption);
    if (ffmpeg_ == NULL) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "无法打开ffmpeg\n");
        exit(0);
    }
    image_t *frame = &(this->ass_img_);
    //ass_set_cache_limits(ass_renderer, 0, 50);

    const size_t buffer_count = config_.video_height_ * config_.video_width_ * 4;

    danmaku::DanmakuHandle handle;
    handle.init_danmaku_screen_dialogue(this->config_);

    wait_queue_ready(this->danmaku_queue_);

    // TODO: stop cond
    while (tm < 1000 * 60 * 60 * 24) {
        update_libass_event(ass_track, handle, this->config_, tm, this->danmaku_queue_);

        ASS_Image *img = ass_render_frame(ass_renderer, ass_track, tm, NULL);
        // clear buffer
        memset(frame->buffer, 0, 1920 * 1080 * 4);
        blend(frame, img);

        tm += ((double)(1000) / (double)(60)); // TODO: 60fps

        auto sz = fwrite(frame->buffer, 1, buffer_count, ffmpeg_);
    }
}

void ffmpeg_render::main_thread() {
    std::thread([this]() { this->run(); }).detach();
}
