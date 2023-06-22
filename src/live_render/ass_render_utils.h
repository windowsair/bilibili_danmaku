#ifndef BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
#define BILIBILI_DANMAKU_ASS_RENDER_UTILS_H

#include "thirdparty/libass/include/ass.h"

struct ass_render_control_base {
  public:
    ass_render_control_base(ASS_Library *lib, ASS_Track *track)
        : ass_library_(lib), track_(track){};

    virtual void create_track(char *ass_style_header, size_t ass_style_header_length) = 0;

    inline auto get_track() {
        return this->track_;
    }

  public:
    ASS_Track *track_;
    ASS_Library *ass_library_;
};

struct danmaku_ass_render_control : public ass_render_control_base {
  public:
    explicit danmaku_ass_render_control(ASS_Library *lib)
        : ass_render_control_base(lib, nullptr) {
    }

    void create_track(char *ass_style_header, size_t ass_style_header_length);
};

struct sc_ass_render_control : public ass_render_control_base {
  public:
    explicit sc_ass_render_control(ASS_Library *lib)
        : ass_render_control_base(lib, nullptr) {
    }

    void create_track(char *ass_style_header, size_t ass_style_header_length);
};

void danmaku_ass_render_control::create_track(char *ass_style_header,
                                              size_t ass_style_header_length) {
    if (this->track_ != nullptr) {
        ass_free_track(track_);
    }

    this->track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, NULL);
}

void sc_ass_render_control::create_track(char *ass_style_header,
                                         size_t ass_style_header_length) {
    if (this->track_ != nullptr) {
        ass_free_track(track_);
    }

    this->track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, NULL);
}

#endif //BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
