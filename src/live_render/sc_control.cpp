﻿#include "sc_control.h"

namespace sc {
ScControl::ScControl(ass::sc_ass_render_control *ass_object,
                     config::live_render_config_t &config)
    : ass_object_(ass_object), width_(config.sc_max_width_),
      screen_height_(config.video_height_), show_range_(config.sc_show_range_),
      corner_radius_(17), font_size_(config.sc_font_size_),
      y_mirror_(config.sc_y_mirror_), x_(config.sc_margin_x_) {
    const int line_top_margin = static_cast<int>(font_size_ / 6.0f);
    max_height_ = y_mirror_
                      ? static_cast<int>(screen_height_ * show_range_) + line_top_margin
                      : static_cast<int>(screen_height_ * show_range_);
    next_show_time_ = INT32_MAX;
    next_fade_out_time_ = INT32_MAX;
    next_end_time_ = INT32_MAX;

    state_ = Default::getInstance();
}

void ScControlState::changeState(ScControl *control, ScControlState *state) {
    control->changeState(state);
}

void ScControl::changeState(ScControlState *state) {
    state_ = state;
}
// TODO: stat information
void ScControl::updateSuperChatEvent(
    ass::sc_ass_render_control *ass_object, config::ass_config_t &config, int base_time,
    int duration_time, moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> *queue,
    live_monitor *monitor) {
    while (auto p = queue->peek()) {
        std::vector<sc::sc_item_t> &sc_list = *p;

        for (auto &item : sc_list) {
            int fade_in_time = item.start_time_;
            ass::SuperChatMessage *pmsg;
            ass::SuperChatMessage msg{item, 0, 0, width_, corner_radius_, font_size_};

            this->sc_list_.push_back(std::move(msg));
            pmsg = &this->sc_list_.back();
            auto it = this->sc_list_.end();
            it--;

            wait_in_deque_.emplace_back(pmsg, it, fade_in_time, 0, 0, 0);
        }

        queue->pop();
    }

    state_->updateSuperChat(this, base_time, duration_time);
}

int ScControl::getItemTotalHeight() {
    int ret = 0;
    for (auto &it : show_deque_) {
        ret += it.total_height();
    }

    return ret;
}

int ScControl::updateExpireItem(int base_time) {
    int ret = 0;
    using show_deque_iter_t = decltype(show_deque_.begin());
    std::vector<show_deque_iter_t> interest_list;

    if (show_deque_.empty()) {
        return 0;
    }

    for (auto it = show_deque_.begin(); it != show_deque_.end(); it++) {
        sc_show_info_t &item = *it;

        if (item.fade_out_time < base_time) {
            interest_list.push_back(it);
        }
    }

    if (interest_list.empty()) {
        return 0;
    }
    ret = static_cast<int>(interest_list.size());

    const int start_time = base_time;
    const int move_left_end_time = start_time + SC_ANIME_FADE_OUT_MOVE_LEFT_TIME;
    const int move_down_end_time = move_left_end_time + SC_ANIME_FADE_OUT_MOVE_DOWN_TIME;
    const int line_top_margin = static_cast<int>(font_size_ / 6.0f);
    int y1, y2;
    std::string ass_str;

    // Initial state -> Item 1 expired.
    // First, Item 1 moves to the left and exits the screen.
    //
    // +----------+      +----------+      +----------+
    // |          |      |          |      |          |
    // | ***2     |      | ***2     |      | ***2     |
    // |          |      |          |      |          |
    // | ***1     | ===> |*1        | ===> |          |
    // |          |      |          |      |          |
    // | ***0     |      | ***0     |      | ***0     |
    // |          |      |          |      |          |
    // +----------+      +----------+      +----------+
    //
    // End state: The remaining items move downward.
    //
    // +----------+      +----------+      +----------+
    // |          |      |          |      |          |
    // | ***2     |      |          |      |          |
    // |          |      | ***2     |      |          |
    // |          | ===> |          | ===> | ***2     |
    // |          |      |          |      |          |
    // | ***0     |      | ***0     |      | ***0     |
    // |          |      |          |      |          |
    // +----------+      +----------+      +----------+
    ass_object_->flush_track();
    for (auto &it : interest_list) {
        sc_show_info_t &item = *it;
        item.fade_out_time = start_time;
        item.end_time = move_left_end_time;

        ass_str = item.msg->getSuperChatAss(x_, item.y, -item.width() - 10, item.y,
                                            start_time, move_left_end_time);
        ass_object_->update_event_line(ass_str);

        fade_out_deque_.push_back(item);
        show_deque_.erase(it);
    }

    y2 = y_mirror_ ? line_top_margin : screen_height_;
    for (auto it = show_deque_.rbegin(); it != show_deque_.rend(); it++) {
        sc_show_info_t &item = *it;

        y1 = item.y;
        if (!y_mirror_) {
            y2 -= item.total_height();
        }
        item.y = y2;
        if (y_mirror_) {
            y2 += item.total_height();
        }

        ass_str = item.msg->getSuperChatAss(x_, y1, x_, y2, move_left_end_time,
                                            move_down_end_time);
        ass_object_->update_event_line(ass_str);
    }

    next_end_time_ = move_down_end_time;
    return ret;
}

int ScControl::addNewItem(std::vector<sc_show_info_t *> &sc_list, int base_time) {
    const int move_up_end_time = base_time + SC_ANIME_FADE_IN_MOVE_UP_TIME;
    const int move_right_end_time = move_up_end_time + SC_ANIME_FADE_IN_MOVE_RIGHT_TIME;
    const int end_time = move_right_end_time + 2 * 60 * 60 * 1000;
    const int line_top_margin = static_cast<int>(font_size_ / 6.0f);
    int y1, y2;
    std::string ass_str;

    ass_object_->flush_track();
    // Initial state -> Add new item.
    // The old items are moved upwards to free up space for new items.
    //
    // +----------+      +----------+      +----------+
    // |          |      |          |      |          |
    // |          |      |          |      | ***1     |
    // |          |      | ***1     |      |          |
    // | ***1     | ===> |          | ===> | ***0     |
    // |          |      | ***0     |      |          |
    // | ***0     |      |          |      |          |
    // |          |      |          |      |          |
    // +----------+      +----------+      +----------+
    //
    // End state: New items move to the right
    //
    // +----------+      +----------+      +----------+
    // |          |      |          |      |          |
    // | ***1     |      | ***1     |      | ***1     |
    // |          |      |          |      |          |
    // | ***0     | ===> | ***0     | ===> | ***0     |
    // |          |      |          |      |          |
    // |          |      |*New       |     | ***New   |
    // |          |      |          |      |          |
    // +----------+      +----------+      +----------+

    // calculate the coordinates of the new items
    y2 = y_mirror_ ? line_top_margin : screen_height_;
    for (auto it = sc_list.rbegin(); it != sc_list.rend(); it++) {
        sc_show_info_t *p = *it;
        if (!y_mirror_) {
            // Fill from bottom to top
            y2 -= p->total_height();
        }
        p->y = y2;
        if (y_mirror_) {
            // Fill from top to bottom
            y2 += p->total_height();
        }
    }
    // calculate the coordinates of the items that already on screen
    // update old items anime
    for (auto it = show_deque_.rbegin(); it != show_deque_.rend(); it++) {
        sc_show_info_t &item = *it;

        y1 = item.y;
        if (!y_mirror_) {
            y2 -= item.total_height();
        }
        item.y = y2;
        if (y_mirror_) {
            y2 += item.total_height();
        }

        ass_str = item.msg->getSuperChatAss(x_, y1, x_, y2, base_time, move_up_end_time);
        ass_object_->update_event_line(ass_str);
    }

    // update new items insert anime
    for (auto p : sc_list) {
        ass_str = p->msg->getSuperChatAss(-10 - p->width(), p->y, x_, p->y,
                                          move_up_end_time, move_right_end_time);
        ass_object_->update_event_line(ass_str);
    }

    // always on screen
    for (auto &item : show_deque_) {
        ass_str = item.msg->getSuperChatAss(x_, item.y, x_, item.y, move_right_end_time,
                                            end_time);
        ass_object_->update_event_line(ass_str);
    }

    // always on screen : new items
    for (auto p : sc_list) {
        p->show_time = move_right_end_time;
        p->fade_out_time = move_right_end_time + getItemAliveTime(p->price());

        ass_str =
            p->msg->getSuperChatAss(x_, p->y, x_, p->y, move_right_end_time, end_time);
        ass_object_->update_event_line(ass_str);
    }

    // update show/wait deque
    for (auto p : sc_list) {
        auto it = std::find_if(wait_in_deque_.begin(), wait_in_deque_.end(),
                               [p](sc_show_info_t &item) { return p == &item; });
        assert(it != wait_in_deque_.end());
        show_deque_.push_back(*p);
        wait_in_deque_.erase(it);
    }

    next_show_time_ = move_right_end_time;
    return 0;
}


int ScControl::getItemAliveTime(int price) {
    if (price < 50) {
        return 60 * 1000;
    } else if (price < 100) {
        return 2 * 60 * 1000;
    } else if (price < 500) {
        return 5 * 60 * 1000;
    } else if (price < 1000) {
        return 30 * 60 * 1000;
    } else if (price < 2000) {
        return 60 * 60 * 1000;
    }

    return 2 * 60 * 60 * 1000;
}

void ScControl::updateNextFadeOutTimeInShowDeque() {
    int min_time = INT32_MAX;
    for (auto &item : show_deque_) {
        min_time = (std::min)(min_time, item.fade_out_time);
    }

    next_fade_out_time_ = min_time;
}

void Default::updateSuperChat(ScControl *control, int base_time, int duration_time) {
    int ret = 0;
    std::vector<sc_show_info_t *> interest_list; // items want to insert

    // fade_out_list_ size must be 0, wait_in_list_ size >= 0
    for (auto it = control->wait_in_deque_.begin();
         it != control->wait_in_deque_.end();) {
        sc_show_info_t &item = *it;
        if (item.fade_in_time < base_time - SC_INSERT_TIMEOUT) {
            // Insert timeout. Remove this item
            control->sc_list_.erase(item.it);
            it = control->wait_in_deque_.erase(it);
            continue;
        } else if (item.fade_in_time <= base_time) {
            interest_list.push_back(&item);
        }

        ++it;
    }

    if (!interest_list.empty() || control->next_fade_out_time_ <= base_time) {
        ret = control->updateExpireItem(base_time);
        if (ret != 0) {
            changeState(control, AnimeFadeOut::getInstance());
            return;
        }
    }

    if (interest_list.empty()) {
        return; // nothing to do
    }

    // Has items to insert, but none of superchat have expired.
    const int max_height = control->max_height_;
    int total_height = control->getItemTotalHeight();
    int wait_list_height = 0;
    bool try_to_insert = true;

    for (auto p : interest_list) {
        wait_list_height += p->total_height();
    }

    if (total_height + wait_list_height > max_height) {
        std::vector<sc_show_info_t *> remove_list;
        for (auto &item : control->show_deque_) {
            if (base_time - item.show_time > SC_MIN_SHOW_TIME) {
                remove_list.push_back(&item);
            }
        }

        // Items that stay the longest are deleted first
        std::sort(remove_list.begin(), remove_list.end(),
                  [](auto a, auto b) { return a->show_time > b->show_time; });

        int removed_height = total_height + wait_list_height;
        for (auto p : remove_list) {
            removed_height -= p->total_height();
            p->fade_out_time = base_time - 10; // mark as expired
            if (removed_height <= max_height) {
                break;
            }
        }

        ret = control->updateExpireItem(base_time);
        if (ret != 0) {
            changeState(control, AnimeFadeOut::getInstance());
            return;
        } else {
            try_to_insert = false;
        }
    }

    // It is possible to try the insertion again,
    // but there is no guarantee that all items will be inserted successfully.
    if (try_to_insert) {
        // Prioritize selecting items with the highest price
        // and maximum length for insertion.
        std::sort(interest_list.begin(), interest_list.end(), [](auto a, auto b) {
            if (a->price() == b->price()) {
                return a->total_height() > b->total_height();
            } else {
                return a->price() > b->price();
            }
        });

        std::vector<sc_show_info_t *> insert_list;
        for (auto p : interest_list) {
            if (total_height + p->total_height() > max_height) {
                break;
            }
            total_height += p->total_height();
            insert_list.push_back(p);
        }

        if (insert_list.empty()) {
            return;
        }

        control->addNewItem(insert_list, base_time);
        changeState(control, AnimeFadeIn::getInstance());
    }
}

void AnimeFadeIn::updateSuperChat(ScControl *control, int base_time, int duration_time) {
    if (base_time < control->next_show_time_) {
        return;
    }

    control->updateNextFadeOutTimeInShowDeque();

    // Items already inserted to show_deque. Nothing to do.
    changeState(control, Default::getInstance());
}

void AnimeFadeOut::updateSuperChat(ScControl *control, int base_time, int duration_time) {
    if (base_time < control->next_end_time_) {
        return;
    }

    // remove unused items
    for (auto it = control->fade_out_deque_.begin();
         it != control->fade_out_deque_.end();) {
        auto &item = *it;
        if (item.end_time < base_time) {
            control->sc_list_.erase(item.it);
            it = control->fade_out_deque_.erase(it);
        } else {
            ++it;
        }
    }

    control->updateNextFadeOutTimeInShowDeque();

    if (control->fade_out_deque_.empty()) {
        changeState(control, Default::getInstance());
    }
}

AnimeFadeIn *AnimeFadeIn::getInstance() {
    static AnimeFadeIn instance{};
    return &instance;
}

AnimeFadeOut *AnimeFadeOut::getInstance() {
    static AnimeFadeOut instance{};
    return &instance;
}

Default *Default::getInstance() {
    static Default instance{};
    return &instance;
}

}; // namespace sc
