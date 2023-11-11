#ifndef BILIBILI_DANMAKU_TEST_SC_COMMON_H
#define BILIBILI_DANMAKU_TEST_SC_COMMON_H

#include <string>

#include "sc_item.h"

#include "thirdparty/libass/include/ass.h"
#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

extern "C" {
int ass_process_events_line(ASS_Track *track, char *str);
}

void libass_init(ASS_Library **ass_library, ASS_Renderer **ass_renderer, int frame_w,
                 int frame_h, bool enable_message_output);

int process_sc_list_from_file(
    std::string &file_path,
    moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> &queue);

#endif //BILIBILI_DANMAKU_TEST_SC_COMMON_H
