#ifndef BILIBILI_DANMAKU_DANMAKU_FILTER_H
#define BILIBILI_DANMAKU_DANMAKU_FILTER_H

#include <string>
#include <vector>

#include "danmaku.h"

#include "thirdparty/re2/re2/re2.h"

class DanmakuFilter {
  public:
    DanmakuFilter() : is_blacklist_used_(false) {
    }
    void filter_init();

    bool danmaku_item_pre_process(danmaku::danmaku_item_t& item);

  private:
    void init_blacklist();

  private:
    bool is_blacklist_used_;
    std::vector<RE2 *> blacklist_regex_;
};

#endif //BILIBILI_DANMAKU_DANMAKU_FILTER_H
