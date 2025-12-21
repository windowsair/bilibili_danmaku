#ifndef BILIBILI_DANMAKU_ASS_RENDER_H
#define BILIBILI_DANMAKU_ASS_RENDER_H

#include "ffmpeg_render.h"
#include "thirdparty/libass/include/ass.h"

namespace ass {
void ass_blend(image_t *frame, ASS_Image *img, uint64_t offset);
}

#endif //BILIBILI_DANMAKU_ASS_RENDER_H
