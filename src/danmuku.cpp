#include <algorithm>
#include <map>

#include "ass.h"
#include "danmuku.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

namespace danmuku {


// TODO: i18n
inline std::string_view trim(std::string_view s) {
    s.remove_prefix(std::min(s.find_first_not_of(" \t\r\v\n"), s.size()));
    s.remove_suffix(std::min(s.size() - s.find_last_not_of(" \t\r\v\n") - 1, s.size()));

    return s;
}

/**
 *
 * @param file_path
 * @param danmuku_all_list
 * @return  0: success
 *          other: fail
 */
int parse_danmuku_xml(std::string file_path,
                      std::vector<danmuku_item_t> &danmuku_all_list,
                      danmuku_info_t &danmuku_info) {

    pugi::xml_document doc;
    pugi::xml_parse_result parse_result = doc.load_file(file_path.c_str());
    if (parse_result.status == pugi::status_end_element_mismatch) {
        // maybe <i> mismatch in tail
    }

    auto root_node = doc.child("i");
    if (!root_node) {
        ////TODO:
        return -1;
    }

    danmuku_info.chat_server_ = std::string(root_node.child("chatserver").text().get());
    danmuku_info.chat_id_ = std::string(root_node.child("chatid").text().get());

    //    <d p="10.625,4,25,14893055,1647777274392,0,281193139,0"
    //       user="..."
    //       raw="...">
    //       快快快快 </d>
    for (auto &d : root_node.children("d")) {
        auto p = std::string(d.attribute("p").value());
        std::string context = std::string(trim(d.text().get()));
        // already escape
        danmuku_all_list.emplace_back(context, p);
    }

    return 0;
}

/**
 * Sort list, get move and pos list
 *
 * @param danmuku_all_list [in]
 * @param danmuku_move_list [out]
 * @param danmuku_pos_list [out]
 * @return
 */
int process_danmuku_list(const std::vector<danmuku_item_t> &danmuku_all_list,
                         std::vector<danmuku_item_t> &danmuku_move_list,
                         std::vector<danmuku_item_t> &danmuku_pos_list) {
    if (danmuku_all_list.empty()) {
        return -1;
    }

    for (auto &item : danmuku_all_list) {
        if (item.danmuku_type_ == static_cast<int>(danmu_type::MOVE)) {
            danmuku_move_list.emplace_back(item);
        } else {
            danmuku_pos_list.emplace_back(item);
        }
    }

    std::sort(danmuku_pos_list.begin(), danmuku_pos_list.end(),
              [](danmuku_item_t &a, danmuku_item_t &b) {
                  return static_cast<bool>((a.start_time_ - b.start_time_) < 0);
              });

    // Promise earliest in time and longest in time
    std::sort(danmuku_move_list.begin(), danmuku_move_list.end(),
              [](danmuku_item_t &a, danmuku_item_t &b) {
                  if (a.start_time_ == b.start_time_) {
                      return static_cast<bool>((a.length_ - b.length_) < 0);
                  } else {
                      return static_cast<bool>((a.start_time_ - b.start_time_) < 0);
                  }
              });

    return 0;
}

/**
 *
 * @param screen_dialogue
 * @param index screen dialogue index
 * @param danmuku danmuku to insert
 * @param ass_result_list
 */
inline void insert_dialogue(std::vector<ass_dialogue_t> &screen_dialogue, int index,
                            danmuku_item_t &danmuku,
                            std::vector<ass_dialogue_t> &ass_result_list) {

    // screen_dialogue focus on: start_time, font_size
    ass_dialogue_t ass = {
        .length_ = danmuku.length_,
        .start_time_ = danmuku.start_time_,
        .font_size_ = danmuku.font_size_,
        .is_valid_ = true,
    };

    screen_dialogue[index] = ass; // replace old danmuku

    ass.context_ = danmuku.context_;
    ass.font_color_ = danmuku.font_color_;
    ass.danmuku_type_ = danmuku.danmuku_type_;
    ass.dialogue_line_index_ = index; // calculate the y-axis coordinates

    ass_result_list.push_back(ass);
}

int process_danmuku_dialogue_pos(std::vector<danmuku_item_t> &danmuku_list,
                                 config::ass_config_t &config,
                                 std::vector<ass_dialogue_t> &ass_result_list) {
    if (danmuku_list.empty() || danmuku_list[0].danmuku_type_ == static_cast<int>(danmu_type::MOVE)) {
        return -1;
    }

    int font_size_scale = config.font_size_ * config.font_scale_;

    const int danmuku_line_count =
        (float)config.video_height_ / (float)font_size_scale * config.danmuku_show_range_;

    std::vector<ass_dialogue_t> top_screen_dialogue(danmuku_line_count);
    std::vector<ass_dialogue_t> bottom_screen_dialogue(danmuku_line_count);
    for (auto &item : top_screen_dialogue) {
        item.is_valid_ = false;
    }
    for (auto &item : bottom_screen_dialogue) {
        item.is_valid_ = false;
    }

    // find a valid location to insert danmuku
    for (auto &item : danmuku_list) {
        int font_size = item.font_size_ * config.font_scale_;
        auto &screen_dialogue =
            item.danmuku_type_ == static_cast<int>(danmu_type::TOP) ? top_screen_dialogue : bottom_screen_dialogue;

        for (int i = 0; i < danmuku_line_count; i++) {
            auto &cur_danmuku_on_screen = screen_dialogue[i];
            if (!cur_danmuku_on_screen.is_valid_) {
                // okay. Just insert.
                insert_dialogue(screen_dialogue, i, item, ass_result_list);
                break;
            } else {
                float cur_danmuku_exit_time =
                    cur_danmuku_on_screen.start_time_ + config.danmuku_pos_time_;
                // staggered timing to ensure no overlay
                if (item.start_time_ > cur_danmuku_exit_time) {
                    insert_dialogue(screen_dialogue, i, item, ass_result_list);
                    break;
                }
            }
        }
    }

    return 0;
}

int process_danmuku_dialogue_move(std::vector<danmuku_item_t> &danmuku_list,
                                  config::ass_config_t &config,
                                  std::vector<ass_dialogue_t> &ass_result_list) {
    if (danmuku_list.empty() || danmuku_list[0].danmuku_type_ != static_cast<int>(danmu_type::MOVE)) {
        return -1;
    }

    const int danmuku_line_count = (float)config.video_height_ /
                                   (float)config.font_size_ * config.danmuku_show_range_;

    std::vector<ass_dialogue_t> screen_dialogue(danmuku_line_count);
    for (auto &item : screen_dialogue) {
        item.is_valid_ = false;
    }

    // find a valid location to insert danmuku
    for (auto &item : danmuku_list) {
        int font_size = item.font_size_ * config.font_scale_;

        // check if there is currently a suitable position on the screen
        for (int i = 0; i < danmuku_line_count; i++) {
            auto &cur_danmuku_on_screen = screen_dialogue[i];
            if (!cur_danmuku_on_screen.is_valid_) {
                // okay. just insert
                insert_dialogue(screen_dialogue, i, item, ass_result_list);
                break;
            } else {
                int cur_start_time = cur_danmuku_on_screen.start_time_;
                int cur_font_size = config.font_scale_ * cur_danmuku_on_screen.font_size_;

                int cur_danmuku_length = cur_danmuku_on_screen.length_ * cur_font_size;

                /*
                   |    danmuku_length   |    <-----------   |    danmuku_length   |
                                         |        screen     |

                    distance  =  screen_length + danmuku_length
                    speed =  distance / danmuku_move_time;


                    case 1: danmuku fully visible(first time)
                                            |    danmuku_length   |
                    |        screen                               |

                    fully_visible_distance = danmuku_length;
                    fully_visible_time_cost =  full_visible_distance /  speed


                    case 2: danmuku fully visible(last time)


                    |    danmuku_length   |
                    |        screen                               |

                    fully_visible_distance = screen_length;
                    fully_visible_time_cost = fully_visible_distance / speed;


                    case 3: danmuku exit
                    |    danmuku_length   |
                                          |        screen                               |

                    exit_distance  =  screen_length + danmuku_length;
                    exit_time_cost =  exit_distance / speed;


                */

                // Exactly the time fully visible (first time)
                float cur_danmuku_full_enter_time_first =
                    (float)(config.danmuku_move_time_ * cur_danmuku_length) /
                    (float)(config.video_width_ + cur_danmuku_length);
                cur_danmuku_full_enter_time_first += cur_start_time;

                float cur_danmuku_exit_time = cur_start_time + config.danmuku_move_time_;
                //

                int new_danmuku_length = font_size * item.length_;

                // Exactly the time fully visible (last time)
                float new_danmuku_enter_time_left =
                    (float)(config.video_width_ * config.danmuku_move_time_) /
                    (float)(config.video_width_ + new_danmuku_length);
                new_danmuku_enter_time_left += item.start_time_;

                // Must be inserted after the previous item is fully visible
                // As an additional condition, we expect that the new danmuku
                // cannot catch up with the previous danmuku.
                if (item.start_time_ >= cur_danmuku_full_enter_time_first &&
                    new_danmuku_enter_time_left >= cur_danmuku_exit_time) {
                    // insert danmuku!
                    insert_dialogue(screen_dialogue, i, item, ass_result_list);
                    break;
                }
            }
        }
    }

    return 0;
}

// do not share config parameter cuz we will change it.
int danmuku_main_process(std::string xml_file, config::ass_config_t config) {
    std::vector<danmuku_item_t> danmuku_all_list, danmuku_move_list, danmuku_pos_list;
    danmuku_info_t danmuku_info;
    int ret = parse_danmuku_xml(xml_file, danmuku_all_list, danmuku_info);
    if (ret != 0) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:无效的XML文件\n",
                   xml_file);
        return -1;
    }
    if (danmuku_all_list.empty()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:未找到有效项目\n",
                   xml_file);
        return 0;
    }

    // get base color, base font size
    using font_size_t = int;
    using font_color_t = int;
    std::map<font_size_t, int> mp_size;
    std::map<font_color_t, int> mp_color;

    for (auto &item : danmuku_all_list) {
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

    int font_color_tmp = mp_color.begin()->first;
    int font_size_tmp = mp_size.begin()->first;

    ret = process_danmuku_list(danmuku_all_list, danmuku_move_list, danmuku_pos_list);


    config.font_size_ = font_size;
    config.font_color_ = font_color;
    config.chat_server_ = danmuku_info.chat_server_;
    config.chat_id_ = danmuku_info.chat_id_;

    std::vector<ass_dialogue_t> ass_result_list;
    ret = process_danmuku_dialogue_move(danmuku_move_list, config, ass_result_list);

    if (ret != 0) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:处理时发生错误\n",
                   xml_file);
        ////TODO:
    }

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

} // namespace danmuku