#ifndef BILIBILI_DANMAKU_TEST_SC_COMMON_H
#define BILIBILI_DANMAKU_TEST_SC_COMMON_H

#include <string>

#include "sc_item.h"

#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

int process_sc_list_from_file(
    std::string &file_path,
    moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> &queue);

#endif //BILIBILI_DANMAKU_TEST_SC_COMMON_H
