#ifndef BILIBILI_DANMAKU_ASS_DANMAKU_H
#define BILIBILI_DANMAKU_ASS_DANMAKU_H

#include "danmaku.h"

namespace ass {
int ass_save_to_file(const std::string &output_file_name,
                     const config::ass_config_t &config,
                     std::vector<danmaku::ass_dialogue_t> &ass_dialogue_list,
                     std::vector<std::string> &ass_sc_list);

std::string rgb2bgr(int rgb);
std::string time2ass(float time);
std::string time2ass(int time);

bool is_custom_ass_file_exist(std::string filename);

std::string get_sc_ass_header(const config::ass_config_t &config);

std::string get_ass_header(const config::ass_config_t &config,
                           std::vector<danmaku::ass_dialogue_t> &ass_dialogue_list);

std::string get_ass_event(const config::ass_config_t &config,
                          danmaku::ass_dialogue_t &item);

} // namespace ass

#endif //BILIBILI_DANMAKU_ASS_DANMAKU_H
