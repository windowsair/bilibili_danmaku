#pragma once
#ifndef BILIBILI_DANMAKU_SC_CONTROL_HPP
#define BILIBILI_DANMAKU_SC_CONTROL_HPP

#include <vector>
#include <deque>
#include <list>

#include "ass_util.hpp"
#include "ass_render_utils.h"
#include "live_monitor.h"
#include "thirdparty/readerwriterqueue/readerwriterqueue.h"


namespace sc {
constexpr int SC_INSERT_TIMEOUT = 10 * 1000;
constexpr int SC_MIN_SHOW_TIME = 10 * 1000;
constexpr int SC_ANIME_FADE_IN_MOVE_UP_TIME = 300;
constexpr int SC_ANIME_FADE_IN_MOVE_RIGHT_TIME = 200;
constexpr int SC_ANIME_FADE_OUT_MOVE_DOWN_TIME = 300;
constexpr int SC_ANIME_FADE_OUT_MOVE_LEFT_TIME = 200;

class ScControlDebugProbe;

typedef struct sc_show_info_ {
    ass::SuperChatMessage *msg;
    std::list<ass::SuperChatMessage>::iterator it;
    int fade_in_time;
    int show_time;
    int fade_out_time;
    int end_time;
    int y;

    sc_show_info_(ass::SuperChatMessage *msg_,
                  std::list<ass::SuperChatMessage>::iterator it_, int fade_in_time_,
                  int show_time_, int fade_out_time_, int end_time_, int y_)
        : msg(msg_), it(it_), fade_in_time(fade_in_time_), show_time(show_time_),
          fade_out_time(fade_out_time_), end_time(end_time_), y(y_) {
    }

    inline int price() {
        return msg->sc_.price_;
    }

    inline int total_height() {
        return msg->total_height_;
    }

    inline int width() {
        return msg->width_;
    }
} sc_show_info_t;

class ScControl;

class ScControlState {
  public:
    ScControlState() = default;
    virtual void updateSuperChat(ScControl *control, int base_time,
                                 int duration_time) = 0;

  protected:
    void changeState(ScControl *control, ScControlState *state);
};

class Default : public ScControlState {
  public:
    static Default *getInstance();
    void updateSuperChat(ScControl *control, int base_time, int duration_time) override;

  protected:
    Default() = default;
};


class AnimeFadeIn : public ScControlState {
  public:
    static AnimeFadeIn *getInstance();
    void updateSuperChat(ScControl *control, int base_time, int duration_time) override;

  protected:
    AnimeFadeIn() = default;
};

class AnimeFadeOut : public ScControlState {
  public:
    static AnimeFadeOut *getInstance();
    void updateSuperChat(ScControl *control, int base_time, int duration_time) override;

  protected:
    AnimeFadeOut() = default;
};

class ScControl {
  public:
    explicit ScControl(ass::sc_ass_render_control *ass_object,
                       config::live_render_config_t &config);

    void
    updateSuperChatEvent(ass::sc_ass_render_control *ass_object,
                         config::ass_config_t &config, int base_time, int duration_time,
                         moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> *queue,
                         live_monitor *monitor);

    friend class ScControlState;
    friend class Default;
    friend class AnimeFadeIn;
    friend class AnimeFadeOut;
    friend class ScControlDebugProbe;

  private:
    int updateExpireItem(int base_time);
    int addNewItem(std::vector<sc_show_info_t *> &sc_list, int base_time);
    int getItemTotalHeight();
    int getItemAliveTime(int price);
    void updateNextFadeOutTimeInShowDeque();

  private:
    void changeState(ScControlState *state);
    ScControlState *state_;


  private:
    ass::sc_ass_render_control *ass_object_;
    // config
    int width_, corner_radius_, font_size_, screen_height_;
    int x_;
    float show_range_;
    int max_height_;
    bool y_mirror_;
    bool sc_price_no_break_;
    /**
     * @next_fade_out_time_: Exceeding this time indicates that an element has expired
     * and is waiting to be removed. Always updated at the end of fade_in and fade_out events.
     */
    int next_show_time_, next_fade_out_time_, next_end_time_;
    std::list<ass::SuperChatMessage> sc_list_;
    std::list<sc_show_info_t> wait_in_deque_;
    std::list<sc_show_info_t> show_deque_;
    std::list<sc_show_info_t> fade_out_deque_;
};


} // namespace sc


#endif //BILIBILI_DANMAKU_SC_CONTROL_HPP
