#ifndef BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
#define BILIBILI_DANMAKU_ASS_RENDER_UTILS_H

#include "thirdparty/libass/include/ass.h"

struct ass_render_control {
  public:
    explicit ass_render_control(ASS_Library *lib)
        : ass_library_(lib), danmaku_track_(nullptr), sc_track_(nullptr) {
    }

    void create_new_sc_track(char *ass_style_header, size_t ass_style_header_length);
    void create_new_danmaku_track(char *ass_style_header, size_t ass_style_header_length);

    inline auto get_sc_track() {
        return this->sc_track_;
    }
    inline auto get_danmaku_track() {
        return this->danmaku_track_;
    }

  public:
    ASS_Track *danmaku_track_;
    ASS_Track *sc_track_;
    ASS_Library *ass_library_;
};

void ass_render_control::create_new_sc_track(char *ass_style_header,
                                             size_t ass_style_header_length) {
    if (this->sc_track_ != nullptr) {
        ass_free_track(sc_track_);
    }

    sc_track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, NULL);
}

void ass_render_control::create_new_danmaku_track(char *ass_style_header,
                                                  size_t ass_style_header_length) {
    if (this->danmaku_track_ != nullptr) {
        ass_free_track(danmaku_track_);
    }

    this->danmaku_track_ =
        ass_read_memory(ass_library_, ass_style_header, ass_style_header_length, NULL);
}

#endif //BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
