#ifndef BILIBILI_DANMAKU_ASS_H
#define BILIBILI_DANMAKU_ASS_H

#include "danmaku.h"

namespace ass {
int ass_render(const std::string &output_file_name, const config::ass_config_t &config,
               std::vector<danmaku::ass_dialogue_t> &ass_dialogue_list);
}

#endif //BILIBILI_DANMAKU_ASS_H
