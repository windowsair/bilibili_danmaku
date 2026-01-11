#include "ass_render_utils.h"
#include "ass_util.hpp"

extern "C" {
int ass_process_events_line(ASS_Track *track, char *str);
}

namespace ass {
void libass_msg_callback(int level, const char *fmt, va_list va, void *data) {
    if (level > 6)
        return;
    printf("libass: ");
    vprintf(fmt, va);
    printf("\n");
}

void libass_no_msg_callback(int level, const char *fmt, va_list va, void *data) {
}

void libass_init(ASS_Library **ass_library, ASS_Renderer **ass_renderer, int frame_w,
                 int frame_h, bool enable_message_output) {
    *ass_library = ass_library_init();
    if (!(*ass_library)) {
        printf("ass_library_init failed!\n");
        std::abort();
    }

    if (enable_message_output) {
        ass_set_message_cb(*ass_library, libass_msg_callback, nullptr);
    } else {
        ass_set_message_cb(*ass_library, libass_no_msg_callback, nullptr);
    }

    ass_set_extract_fonts(*ass_library, 1);

    *ass_renderer = ass_renderer_init(*ass_library);
    if (!(*ass_renderer)) {
        printf("ass_renderer_init failed!\n");
        std::abort();
    }

    ass_set_frame_size(*ass_renderer, frame_w, frame_h);
    ass_set_fonts(*ass_renderer, nullptr, "sans-serif", ASS_FONTPROVIDER_AUTODETECT,
                  nullptr, 1);
}

SuperChatRenderFactory::SuperChatRenderFactory(const config::ass_config_t &config) {
    if (!config.sc_enable_)
        return;
    ass::libass_init(&ass_library_, &ass_renderer_, config.video_width_,
                     config.video_height_, false);

    auto copy_cfg = config;
    copy_cfg.font_size_ = config.sc_font_size_;
    copy_cfg.font_alpha_ = config.sc_alpha_;
    auto sc_ass_header_str = ass::get_sc_ass_header(copy_cfg);

    sc_tracker_ =
        ass_read_memory(ass_library_, const_cast<char *>(sc_ass_header_str.c_str()),
                        sc_ass_header_str.size(), nullptr);
    ass::TextProcess::Init(ass_library_, ass_renderer_, sc_tracker_);
}

SuperChatRenderFactory::~SuperChatRenderFactory() {
    if (sc_tracker_)
        ass_free_track(sc_tracker_);
    if (ass_renderer_)
        ass_renderer_done(ass_renderer_);
    if (ass_library_)
        ass_library_done(ass_library_);
}

void danmaku_ass_render_control::create_track(char *ass_style_header,
                                              size_t ass_style_header_length) {
    if (track_ != nullptr) {
        ass_free_track(track_);
    }

    ass_style_header_ = std::string(ass_style_header, ass_style_header_length);
    track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, nullptr);
}

void sc_ass_render_control::create_track(char *ass_style_header,
                                         size_t ass_style_header_length) {
    if (track_ != nullptr) {
        ass_free_track(track_);
    }

    ass_style_header_ = std::string(ass_style_header, ass_style_header_length);
    track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, nullptr);
}

void sc_ass_render_control::flush_track() {
    if (track_ != nullptr) {
        ass_free_track(track_);
    }

    assert(!ass_style_header_.empty());
    track_ = ass_read_memory(ass_library_, const_cast<char *>(ass_style_header_.c_str()),
                             ass_style_header_.size(), nullptr);
}

ass_render_control_base::ass_render_control_base(ASS_Library *lib, ASS_Track *track)
    : ass_library_(lib), track_(track){};

ass_render_control_base::~ass_render_control_base() {
    if (track_) {
        ass_free_track(track_);
    }
}

void ass_render_control_base::update_event_line(std::string &str) {
    ass_process_events_line(track_, const_cast<char *>(str.c_str()));
}

void ass_render_control_base::update_event_line(std::vector<std::string> &line_list) {
    for (auto &line : line_list) {
        line.push_back('\0');
        ass_process_events_line(track_, const_cast<char *>(line.c_str()));
    }
}

danmaku_ass_render_control::danmaku_ass_render_control(ASS_Library *lib)
    : ass_render_control_base(lib, nullptr) {
}

sc_ass_render_control::sc_ass_render_control(ASS_Library *lib)
    : ass_render_control_base(lib, nullptr) {
}

} // namespace ass