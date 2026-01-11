#ifndef BILIBILI_DANMAKU_ASS_RENDER_H
#define BILIBILI_DANMAKU_ASS_RENDER_H

#include "ffmpeg_render.h"
#include "thirdparty/libass/include/ass.h"

#define TO_R(c) ((c) >> 24)
#define TO_G(c) (((c) >> 16) & 0xFF)
#define TO_B(c) (((c) >> 8) & 0xFF)
#define TO_A(c) ((c) & 0xFF)

namespace ass {
void ass_blend(image_t *frame, ASS_Image *img, uint64_t offset);
void ass_blend_ssse3(image_t *frame, ASS_Image *img, uint64_t offset);
void ass_blend_avx2(image_t *frame, ASS_Image *img, uint64_t offset);
}

#endif //BILIBILI_DANMAKU_ASS_RENDER_H
