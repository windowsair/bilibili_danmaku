#ifndef BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
#define BILIBILI_DANMAKU_ASS_RENDER_UTILS_H

#include <string>
#include <vector>

#include "ass_config.h"
#include "thirdparty/libass/include/ass.h"

namespace ass {
void libass_msg_callback(int level, const char *fmt, va_list va, void *data);
void libass_no_msg_callback(int level, const char *fmt, va_list va, void *data);
void libass_init(ASS_Library **ass_library, ASS_Renderer **ass_renderer, int frame_w,
                 int frame_h, bool enable_message_output);

class SuperChatRenderFactory {
  public:
    explicit SuperChatRenderFactory(const config::ass_config_t &config);
    ~SuperChatRenderFactory();

    inline auto get_ass_library() {
        return ass_library_;
    }

    inline auto get_ass_renderer() {
        return ass_renderer_;
    }

  private:
    ASS_Library *ass_library_ = nullptr;
    ASS_Renderer *ass_renderer_ = nullptr;
    ASS_Track *sc_tracker_ = nullptr;
};

class ass_render_control_base {
  public:
    ass_render_control_base(ASS_Library *lib, ASS_Track *track);
    ~ass_render_control_base();

    virtual void create_track(char *ass_style_header, size_t ass_style_header_length) = 0;
    [[nodiscard]] inline auto get_track() const {
        return track_;
    }
    virtual void update_event_line(std::string &str);
    virtual void update_event_line(std::vector<std::string> &line_list);

  public:
    ASS_Track *track_;
    ASS_Library *ass_library_;
    std::string ass_style_header_;
};

class danmaku_ass_render_control : public ass_render_control_base {
  public:
    explicit danmaku_ass_render_control(ASS_Library *lib);

    void create_track(char *ass_style_header, size_t ass_style_header_length) override;
};

class sc_ass_render_control : public ass_render_control_base {
  public:
    explicit sc_ass_render_control(ASS_Library *lib);

    void create_track(char *ass_style_header, size_t ass_style_header_length) override;
    virtual void flush_track();
};
}; // namespace ass

#endif //BILIBILI_DANMAKU_ASS_RENDER_UTILS_H
