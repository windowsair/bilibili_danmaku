#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define ASS_TARGET_x86
#include <immintrin.h>
#include <smmintrin.h>

#include "ass_render.h"

namespace ass {
#define ZERO_MASK ((char)0xff)

inline __m128i _mm_fast_div_255_epu16(__m128i x) {
    return _mm_srli_epi16(
        _mm_adds_epu16(x, _mm_srli_epi16(_mm_adds_epu16(x, _mm_set1_epi16(0x0101)), 8)),
        8);
}

inline void blend_single_ssse3(image_t *frame, ASS_Image *img, uint64_t offset) {
    int x, y, count;
    unsigned short opacity = 255 - TO_A(img->color);
    unsigned short r = TO_R(img->color);
    unsigned short g = TO_G(img->color);
    unsigned short b = TO_B(img->color);

    const int times = img->w / 2;

    unsigned char *src;
    unsigned char *dst, *now_dst;

    __m128i zeros = _mm_setzero_si128();

    src = img->bitmap;
    dst = (frame->buffer + offset) + img->dst_y * frame->stride + img->dst_x * 4;
    for (y = 0; y < img->h; ++y) {
        for (x = 0, now_dst = dst, count = 0; count < times; count++) {
            __m128i rgb_v = _mm_set_epi16(255, b, g, r, 255, b, g, r);

            __m128i k_v = _mm_cvtsi32_si128(*(uint16_t *)(src + x));
            const __m128i mask_rgb =
                _mm_setr_epi8(0, ZERO_MASK, 0, ZERO_MASK, 0, ZERO_MASK, 0, ZERO_MASK, 1,
                              ZERO_MASK, 1, ZERO_MASK, 1, ZERO_MASK, 1, ZERO_MASK);
            k_v = _mm_shuffle_epi8(k_v, mask_rgb);

            __m128i high_k1 = _mm_set1_epi16(opacity);
            k_v = _mm_mullo_epi16(k_v, high_k1);
            k_v = _mm_fast_div_255_epu16(k_v);

            __m128i mul_1 = _mm_mullo_epi16(k_v, rgb_v);

            __m128i sub_max = _mm_set1_epi16(255);
            __m128i k_v2 = _mm_sub_epi16(sub_max, k_v);

            __m128i dst_v = _mm_loadl_epi64((__m128i *)(now_dst));
            dst_v = _mm_unpacklo_epi8(dst_v, zeros);

            __m128i mul_2 = _mm_mullo_epi16(k_v2, dst_v);
            __m128i res1 = _mm_add_epi16(mul_1, mul_2);
            __m128i res2 = _mm_fast_div_255_epu16(res1);

            res2 = _mm_packus_epi16(res2, zeros);

            _mm_storeu_si64(now_dst, res2);

            x += 2;
            now_dst += 8;
        }
        if (img->w & 1) {
            uint32_t k = ((uint32_t)src[x]) * opacity / 255;
            // possible endianness problems...
            // would anyone actually use big endian machine??
            now_dst[0] = (k * r + (255 - k) * now_dst[0]) / (1 * 255);
            now_dst[1] = (k * g + (255 - k) * now_dst[1]) / (1 * 255);
            now_dst[2] = (k * b + (255 - k) * now_dst[2]) / (1 * 255);
            now_dst[3] = (k * 255 + (255 - k) * now_dst[3]) / (1 * 255);
        }

        src += img->stride;
        dst += frame->stride;
    }
}

void ass_blend_ssse3(image_t *frame, ASS_Image *img, uint64_t offset) {
    while (img) {
        blend_single_ssse3(frame, img, offset);
        img = img->next;
    }
}

} // namespace ass

#endif
