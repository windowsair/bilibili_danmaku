#ifndef BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
#define BILIBILI_DANMAKU_ASS_RENDER_UTILS_H

#include <string>
#include <vector>

#include "thirdparty/libass/include/ass.h"

extern "C" {
int ass_process_events_line(ASS_Track *track, char *str);
}

namespace ass {
class ass_render_control_base {
  public:
    ass_render_control_base(ASS_Library *lib, ASS_Track *track)
        : ass_library_(lib), track_(track){};

    virtual void create_track(char *ass_style_header, size_t ass_style_header_length) = 0;

    inline auto get_track() {
        return track_;
    }

    inline virtual void update_event_line(std::string &str) {
        ass_process_events_line(track_, const_cast<char *>(str.c_str()));
    }

    inline virtual void update_event_line(std::vector<std::string> &line_list) {
        for (auto &line : line_list) {
            line.push_back('\0');
            ass_process_events_line(track_, const_cast<char *>(line.c_str()));
        }
    }

  public:
    ASS_Track *track_;
    ASS_Library *ass_library_;
    std::string ass_style_header_;
};

class danmaku_ass_render_control : public ass_render_control_base {
  public:
    explicit danmaku_ass_render_control(ASS_Library *lib)
        : ass_render_control_base(lib, nullptr) {
    }

    void create_track(char *ass_style_header, size_t ass_style_header_length);
};

class sc_ass_render_control : public ass_render_control_base {
  public:
    explicit sc_ass_render_control(ASS_Library *lib)
        : ass_render_control_base(lib, nullptr) {
    }

    virtual void create_track(char *ass_style_header, size_t ass_style_header_length);

    virtual void flush_track();
};

inline void danmaku_ass_render_control::create_track(char *ass_style_header,
                                                     size_t ass_style_header_length) {
    if (track_ != nullptr) {
        ass_free_track(track_);
    }

    ass_style_header_ = std::string(ass_style_header, ass_style_header_length);
    track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, NULL);
}

inline void sc_ass_render_control::create_track(char *ass_style_header,
                                                size_t ass_style_header_length) {
    if (track_ != nullptr) {
        ass_free_track(track_);
    }

    ass_style_header_ = std::string(ass_style_header, ass_style_header_length);
    track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, NULL);
}

inline void sc_ass_render_control::flush_track() {
    if (track_ != nullptr) {
        ass_free_track(track_);
    }

    assert(ass_style_header_.size() > 0);
    track_ = ass_read_memory(ass_library_, const_cast<char *>(ass_style_header_.c_str()),
                             ass_style_header_.size(), NULL);
}

}; // namespace ass


#endif //BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
