#include <algorithm>
#include <map>

#include "ass_danmaku.h"
#include "danmaku.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

namespace danmaku {

// TODO: i18n
inline std::string_view trim(std::string_view s) {
    s.remove_prefix(std::min(s.find_first_not_of(" \t\r\v\n"), s.size()));
    s.remove_suffix(std::min(s.size() - s.find_last_not_of(" \t\r\v\n") - 1, s.size()));

    return s;
}

/**
 *
 * @param file_path
 * @param danmaku_all_list
 * @return  0: success
 *          other: fail
 */
int DanmakuHandle::parse_danmaku_xml(pugi::xml_document &doc, pugi::xml_parse_result &parse_result,
                      std::string file_path,
                      std::vector<danmaku_item_t> &danmaku_all_list,
                      danmaku_info_t &danmaku_info) {

    parse_result = doc.load_file(file_path.c_str());
    if (parse_result.status == pugi::status_end_element_mismatch) {
        // maybe <i> mismatch in tail
    }

    auto root_node = doc.child("i");
    if (!root_node) {
        ////TODO:
        return -1;
    }

    danmaku_info.chat_server_ = std::string(root_node.child("chatserver").text().get());
    danmaku_info.chat_id_ = std::string(root_node.child("chatid").text().get());

    //    <d p="10.625,4,25,14893055,1647777274392,0,281193139,0"
    //       user="..."
    //       raw="...">
    //       快快快快 </d>
    for (auto &d : root_node.children("d")) {
        const char *p = d.attribute("p").value();

        //// FIXME: escape space
        char *origin_context = const_cast<char *>(d.text().get());

        if (*origin_context == '\0') {
            continue; // does not have value
        }

        std::string trim_context = std::string(trim(origin_context));
        memcpy(origin_context, trim_context.c_str(),
               trim_context.size() + 1); // with "\0"

        // already escape
        danmaku_all_list.emplace_back(origin_context, p);
    }

    return 0;
}

/**
 * Sort list, get move and pos list
 *
 * @param danmaku_all_list [in]
 * @param danmaku_move_list [out]
 * @param danmaku_pos_list [out]
 * @return
 */
int DanmakuHandle::process_danmaku_list(const std::vector<danmaku_item_t> &danmaku_all_list,
                         std::vector<danmaku_item_t> &danmaku_move_list,
                         std::vector<danmaku_item_t> &danmaku_pos_list) {
    if (danmaku_all_list.empty()) {
        return -1;
    }

    for (auto &item : danmaku_all_list) {
        if (item.danmaku_type_ == static_cast<int>(danmu_type::MOVE)) {
            danmaku_move_list.emplace_back(item);
        } else {
            danmaku_pos_list.emplace_back(item);
        }
    }

    std::sort(danmaku_pos_list.begin(), danmaku_pos_list.end(),
              [](danmaku_item_t &a, danmaku_item_t &b) {
                  return static_cast<bool>((a.start_time_ - b.start_time_) < 0);
              });

    // Promise earliest in time and longest in time
    std::sort(danmaku_move_list.begin(), danmaku_move_list.end(),
              [](danmaku_item_t &a, danmaku_item_t &b) {
                  return static_cast<bool>((a.start_time_ - b.start_time_) < 0);
              });

    const size_t sz = danmaku_move_list.size();
    bool sort_fun = false;
    for (size_t i = 0; i < sz - 1;) {
        size_t j;
        for (j = i + 1; j < sz; j++) {
            if (danmaku_move_list[i].start_time_ != danmaku_move_list[j].start_time_) {
                break;
            }
        }

        if (j - i > 1) {
            sort_fun = !sort_fun;
            if (sort_fun)
                std::sort(danmaku_move_list.begin() + i, danmaku_move_list.begin() + j,
                      [](danmaku_item_t &a, danmaku_item_t &b) {
                          return a.length_ > b.length_;
                      });
            else {
                std::sort(danmaku_move_list.begin() + i, danmaku_move_list.begin() + j,
                          [](danmaku_item_t &a, danmaku_item_t &b) {
                              return a.length_ < b.length_;
                          });
            }
        }

        i = j;
    }

#ifdef TEST_MOVE_LIST_SORT
    for (int i = 0; i < danmaku_move_list.size(); i++) {
        for (int j = i + 1; j < danmaku_move_list.size(); j++) {
            if (danmaku_move_list[i].start_time_ == danmaku_move_list[j].start_time_) {
                if (danmaku_move_list[i].length_ < danmaku_move_list[j].length_) {
                    std::abort();
                }
            }
        }
    }
#endif

    return 0;
}


void DanmakuHandle::init_danmaku_screen_dialogue(const config::ass_config_t &config) {
    this->danmaku_line_count_ = (float)config.video_height_ *
                                   (float)config.danmaku_show_range_ /
                                   ((float)config.font_size_ * (float)config.font_scale_);

    this->top_screen_dialogue_.resize(danmaku_line_count_);
    this->bottom_screen_dialogue_.resize(danmaku_line_count_);
    this->move_screen_dialogue_.resize(danmaku_line_count_);

    for (auto &item : this->top_screen_dialogue_) {
        item.is_valid_ = false;
    }
    for (auto &item : this->bottom_screen_dialogue_) {
        item.is_valid_ = false;
    }
    for (auto &item : this->move_screen_dialogue_) {
        item.is_valid_ = false;
    }


}

/**
 *
 * @param screen_dialogue
 * @param index screen dialogue index
 * @param danmaku danmaku to insert
 * @param ass_result_list
 */
inline void insert_dialogue(std::vector<ass_dialogue_t> &screen_dialogue, int index,
                            danmaku_item_t &danmaku,
                            std::vector<ass_dialogue_t> &ass_result_list) {

    // screen_dialogue focus on: start_time, font_size
    ass_dialogue_t ass = {
        .length_ = danmaku.length_,
        .start_time_ = danmaku.start_time_,
        .font_size_ = danmaku.font_size_,
        .is_valid_ = true,
    };

    screen_dialogue[index] = ass; // replace old danmaku

    ass.context_ = danmaku.context_;
    ass.font_color_ = danmaku.font_color_;
    ass.danmaku_type_ = danmaku.danmaku_type_;
    ass.dialogue_line_index_ = index; // calculate the y-axis coordinates

    ass_result_list.push_back(ass);
}

int DanmakuHandle::process_danmaku_dialogue_pos(std::vector<danmaku_item_t> &danmaku_list,
                                 const config::ass_config_t &config,
                                 std::vector<ass_dialogue_t> &ass_result_list) {
    if (danmaku_list.empty()) {
        return 0;
    }

    if (danmaku_list[0].danmaku_type_ == static_cast<int>(danmu_type::MOVE)) {
        return -1;
    }

    if (config.danmaku_pos_time_ < 0) {
        return 0; // ignore danmaku
    }


    // find a valid location to insert danmaku
    for (auto &item : danmaku_list) {
        auto &screen_dialogue = item.danmaku_type_ == static_cast<int>(danmu_type::TOP)
                                    ? top_screen_dialogue_
                                    : bottom_screen_dialogue_;

        for (int i = 0; i < danmaku_line_count_; i++) {
            auto &cur_danmaku_on_screen = screen_dialogue[i];
            if (!cur_danmaku_on_screen.is_valid_) {
                // okay. Just insert.
                insert_dialogue(screen_dialogue, i, item, ass_result_list);
                break;
            } else {
                float cur_danmaku_exit_time =
                    cur_danmaku_on_screen.start_time_ + config.danmaku_pos_time_;
                // staggered timing to ensure no overlay
                if (item.start_time_ > cur_danmaku_exit_time) {
                    insert_dialogue(screen_dialogue, i, item, ass_result_list);
                    break;
                }
            }
        }
    }

    return 0;
}

int DanmakuHandle::process_danmaku_dialogue_move(std::vector<danmaku_item_t> &danmaku_list,
                                  const config::ass_config_t &config,
                                  std::vector<ass_dialogue_t> &ass_result_list) {
    if (danmaku_list.empty()) {
        return 0;
    }

    if (danmaku_list[0].danmaku_type_ != static_cast<int>(danmu_type::MOVE)) {
        return -1;
    }

    if (config.danmaku_move_time_ < 0) {
        return 0; // ignore danmaku
    }


    // find a valid location to insert danmaku
    for (auto &item : danmaku_list) {
        int font_size = item.font_size_ * config.font_scale_;

        // check if there is currently a suitable position on the screen
        for (int i = 0; i < danmaku_line_count_; i++) {
            auto &cur_danmaku_on_screen = move_screen_dialogue_[i];
            if (!cur_danmaku_on_screen.is_valid_) {
                // okay. just insert
                insert_dialogue(move_screen_dialogue_, i, item, ass_result_list);
                break;
            } else {
                float cur_start_time = cur_danmaku_on_screen.start_time_;
                int cur_font_size = config.font_scale_ * cur_danmaku_on_screen.font_size_;

                int cur_danmaku_length = cur_danmaku_on_screen.length_ * cur_font_size;

                /*
                   |    danmaku_length   |    <-----------   |    danmaku_length   |
                                         |        screen     |

                    distance  =  screen_length + danmaku_length
                    speed =  distance / danmaku_move_time;


                    case 1: danmaku fully visible(first time)
                                            |    danmaku_length   |
                    |        screen                               |

                    fully_visible_distance = danmaku_length;
                    fully_visible_time_cost =  full_visible_distance /  speed


                    case 2: danmaku fully visible(last time)


                    |    danmaku_length   |
                    |        screen                               |

                    fully_visible_distance = screen_length;
                    fully_visible_time_cost = fully_visible_distance / speed;


                    case 3: danmaku exit
                    |    danmaku_length   |
                                          |        screen                               |

                    exit_distance  =  screen_length + danmaku_length;
                    exit_time_cost =  exit_distance / speed;


                */

                // Exactly the time fully visible (first time)
                float cur_danmaku_full_enter_time_first =
                    (float)(config.danmaku_move_time_ * cur_danmaku_length) /
                    (float)(config.video_width_ + cur_danmaku_length);
                cur_danmaku_full_enter_time_first += cur_start_time;

                float cur_danmaku_exit_time = cur_start_time + config.danmaku_move_time_;
                //

                int new_danmaku_length = font_size * item.length_;

                // Exactly the time fully visible (last time)
                float new_danmaku_enter_time_left =
                    (float)(config.video_width_ * config.danmaku_move_time_) /
                    (float)(config.video_width_ + new_danmaku_length);
                new_danmaku_enter_time_left += item.start_time_;

                // Must be inserted after the previous item is fully visible
                // As an additional condition, we expect that the new danmaku
                // cannot catch up with the previous danmaku.
                if (item.start_time_ > cur_danmaku_full_enter_time_first &&
                    new_danmaku_enter_time_left > cur_danmaku_exit_time) {
                    // insert danmaku!
                    insert_dialogue(move_screen_dialogue_, i, item, ass_result_list);
                    break;
                }
            }
        }
    }

    return 0;
}

// do not share config parameter cuz we will change it.
int DanmakuHandle::danmaku_main_process(std::string xml_file, config::ass_config_t config) {
    std::vector<danmaku_item_t> danmaku_all_list, danmaku_move_list, danmaku_pos_list;
    danmaku_info_t danmaku_info;
    pugi::xml_document doc;
    pugi::xml_parse_result parse_result;

    int ret =
        parse_danmaku_xml(doc, parse_result, xml_file, danmaku_all_list, danmaku_info);
    if (ret != 0) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:无效的XML文件\n",
                   xml_file);
        return -1;
    }
    if (danmaku_all_list.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:未找到有效项目\n",
                   xml_file);
        return 0;
    }

    // get base color, base font size
    using font_size_t = int;
    using font_color_t = int;
    std::map<font_size_t, int> mp_size;
    std::map<font_color_t, int> mp_color;

    for (auto &item : danmaku_all_list) {
        mp_size[item.font_size_]++;
        mp_color[item.font_color_]++;
    }

    int font_color =
        std::max_element(mp_color.begin(), mp_color.end(),
                         [](const auto &x, const auto &y) { return x.second < y.second; })
            ->first;
    int font_size =
        std::max_element(mp_size.begin(), mp_size.end(),
                         [](const auto &x, const auto &y) { return x.second < y.second; })
            ->first;

    ret = process_danmaku_list(danmaku_all_list, danmaku_move_list, danmaku_pos_list);

    config.font_size_ = font_size;
    config.font_color_ = font_color;
    config.chat_server_ = danmaku_info.chat_server_;
    config.chat_id_ = danmaku_info.chat_id_;

    std::vector<ass_dialogue_t> ass_result_list;

    init_danmaku_screen_dialogue(config);

    if (config.danmaku_move_time_ > 0) {
        process_danmaku_dialogue_move(danmaku_move_list, config, ass_result_list);
    }

    if (config.danmaku_pos_time_ > 0 ) {
        process_danmaku_dialogue_pos(danmaku_pos_list, config, ass_result_list);
    }


    // Ass file output

    // "abc.xml" => "abc.ass"
    auto xml_index = xml_file.find_last_of(".xml");
    std::string ass_file;
    if (xml_index != std::string::npos) {
        //  .xml
        //     |------>(xml_index)
        ass_file = xml_file.substr(0, xml_index - 3) + ".ass"; // 3: strlen(".xm")
    } else {
        // oops... not found
        ass_file = xml_file + ".ass";
    }

    ass::ass_render(ass_file, config, ass_result_list);

    return 0;
}

} // namespace danmaku