#ifndef BILIBILI_DANMUKU_ASS_H
#define BILIBILI_DANMUKU_ASS_H

#include "danmuku.h"

namespace ass {
int ass_render(const std::string &output_file_name, const config::ass_config_t &config,
               std::vector<danmuku::ass_dialogue_t> &ass_dialogue_list);
}

#endif //BILIBILI_DANMUKU_ASS_H
