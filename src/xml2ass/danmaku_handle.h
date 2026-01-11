#ifndef BILIBILI_DANMAKU_HANDLE_H
#define BILIBILI_DANMAKU_HANDLE_H

#include <vector>
#include <string>

#include "danmaku.h"
#include "danmaku_filter.h"
#include "sc_item.h"
#include "ass_render_utils.h"

namespace danmaku {

class DanmakuHandle {
  public:
    DanmakuHandle() : danmaku_line_count_(0) {
    }

    void init_danmaku_screen_dialogue(const config::ass_config_t &config);

    int parse_danmaku_xml(pugi::xml_document &doc, pugi::xml_parse_result &parse_result,
                          std::string file_path, DanmakuFilter &filter,
                          std::vector<danmaku_item_t> &danmaku_all_list,
                          std::vector<sc::sc_item_t> &sc_list,
                          danmaku_info_t &danmaku_info);
    int process_danmaku_list(const std::vector<danmaku_item_t> &danmaku_all_list,
                             std::vector<danmaku_item_t> &danmaku_move_list,
                             std::vector<danmaku_item_t> &danmaku_pos_list);

    // version 1 -> std::vector<danmaku_item_t>
    // version 2 -> std::vector< std::reference_wrapper<danmaku_item_t> >
    template <class T>
    int process_danmaku_dialogue_pos(T &danmaku_list, const config::ass_config_t &config,
                                     std::vector<ass_dialogue_t> &ass_result_list);

    template <class T>
    int process_danmaku_dialogue_move(T &danmaku_list, const config::ass_config_t &config,
                                      std::vector<ass_dialogue_t> &ass_result_list);

    int danmaku_main_process(std::string xml_file, config::ass_config_t &config,
                             DanmakuFilter &filter,
                             ass::SuperChatRenderFactory &sc_factory);

    float get_max_danmaku_end_time(int move_time, int pos_time);

  private:
    int danmaku_line_count_;
    std::vector<ass_dialogue_t> top_screen_dialogue_;
    std::vector<ass_dialogue_t> bottom_screen_dialogue_;
    std::vector<ass_dialogue_t> move_screen_dialogue_;
};

} // namespace danmaku


#endif