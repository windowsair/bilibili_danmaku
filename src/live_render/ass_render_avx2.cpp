#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define ASS_TARGET_x86
#include <immintrin.h>
#include <smmintrin.h>

#include "ass_render.h"

namespace ass {
inline __m256i _mm256_fast_div_255_epu16(__m256i value) {
    return _mm256_srli_epi16(
        _mm256_add_epi16(_mm256_add_epi16(value, _mm256_set1_epi16(1)),
                         _mm256_srli_epi16(value, 8)),
        8);
}

inline __m128i _mm_fast_div_255_epu16(__m128i x) {
    return _mm_srli_epi16(
        _mm_adds_epu16(x, _mm_srli_epi16(_mm_adds_epu16(x, _mm_set1_epi16(0x0101)), 8)),
        8);
}

#define ZERO_MASK ((char)0xff)

void blend_single_avx2(image_t *frame, ASS_Image *img, uint64_t offset) {
    int x, y, count;
    unsigned short opacity = 255 - TO_A(img->color);
    unsigned short r = TO_R(img->color);
    unsigned short g = TO_G(img->color);
    unsigned short b = TO_B(img->color);

    const int times = img->w / 4;

    const unsigned char *src;
    unsigned char *dst, *now_dst;

    const __m256i zeros = _mm256_setzero_si256();
    const __m128i zeros_128 = _mm_setzero_si128();

    src = (const unsigned char *)img->bitmap;
    dst = (frame->buffer + offset) + img->dst_y * frame->stride + img->dst_x * 4;

    for (y = 0; y < img->h; ++y) {
        now_dst = dst;
        x = 0;

        for (count = 0; count < times; count++) {

            __m256i rgb_v =
                _mm256_set_epi16(255, b, g, r, 255, b, g, r, 255, b, g, r, 255, b, g, r);
            __m256i k_v = _mm256_broadcastd_epi32(_mm_cvtsi32_si128(*(int *)(src + x)));

            const __m256i rgba_mask =
                _mm256_setr_epi8(0, ZERO_MASK, 0, ZERO_MASK, 0, ZERO_MASK, 0, ZERO_MASK,
                                 1, ZERO_MASK, 1, ZERO_MASK, 1, ZERO_MASK, 1, ZERO_MASK,
                                 2, ZERO_MASK, 2, ZERO_MASK, 2, ZERO_MASK, 2, ZERO_MASK,
                                 3, ZERO_MASK, 3, ZERO_MASK, 3, ZERO_MASK, 3, ZERO_MASK);

            k_v = _mm256_shuffle_epi8(k_v, rgba_mask);

            __m256i op_v = _mm256_set1_epi16(opacity);
            k_v = _mm256_mullo_epi16(k_v, op_v);
            k_v = _mm256_fast_div_255_epu16(k_v);

            __m256i mul1 = _mm256_mullo_epi16(k_v, rgb_v);
            __m256i sub255 = _mm256_set1_epi16(255);
            __m256i k2 = _mm256_sub_epi16(sub255, k_v);

            __m128i dst128 = _mm_loadu_si128((__m128i *)now_dst);
            __m256i dst_v = _mm256_cvtepu8_epi16(dst128);

            __m256i mul2 = _mm256_mullo_epi16(k2, dst_v);

            __m256i sum = _mm256_add_epi16(mul1, mul2);

            __m256i res = _mm256_fast_div_255_epu16(sum);

            __m256i pack = _mm256_packus_epi16(res, zeros);
            pack = _mm256_permute4x64_epi64(pack, 0xD8);
            _mm_storeu_si128((__m128i *)now_dst, _mm256_castsi256_si128(pack));

            x += 4;
            now_dst += 16;
        }

        int remain = img->w & 3;
        if (remain >= 2) {
            remain -= 2;
            __m128i rgb_v = _mm_set_epi16(255, b, g, r, 255, b, g, r);

            __m128i low_k = _mm_set1_epi16(src[x]);
            __m128i high_k1 = _mm_set1_epi16(src[x + 1]);
            __m128i k_v = _mm_unpackhi_epi64(low_k, high_k1);
            high_k1 = _mm_set1_epi16(opacity);
            k_v = _mm_mullo_epi16(k_v, high_k1);
            k_v = _mm_fast_div_255_epu16(k_v);

            __m128i mul_1 = _mm_mullo_epi16(k_v, rgb_v);

            __m128i sub_max = _mm_set1_epi16(255);
            __m128i k_v2 = _mm_sub_epi16(sub_max, k_v);

            __m128i dst_v = _mm_loadl_epi64((__m128i *)(now_dst));
            dst_v = _mm_unpacklo_epi8(dst_v, zeros_128);

            __m128i mul_2 = _mm_mullo_epi16(k_v2, dst_v);
            __m128i res1 = _mm_add_epi16(mul_1, mul_2);
            __m128i res2 = _mm_fast_div_255_epu16(res1);

            res2 = _mm_packus_epi16(res2, zeros_128);

            _mm_storeu_si64(now_dst, res2);
            x += 2;
            now_dst += 8;
        }

        for (int i = 0; i < remain; i++) {
            uint32_t k = ((uint32_t)src[x + i]) * opacity / 255;
            now_dst[i * 4 + 0] = (k * r + (255 - k) * now_dst[i * 4 + 0]) / 255;
            now_dst[i * 4 + 1] = (k * g + (255 - k) * now_dst[i * 4 + 1]) / 255;
            now_dst[i * 4 + 2] = (k * b + (255 - k) * now_dst[i * 4 + 2]) / 255;
            now_dst[i * 4 + 3] = (k * 255 + (255 - k) * now_dst[i * 4 + 3]) / 255;
        }

        src += img->stride;
        dst += frame->stride;
    }
}

void ass_blend_avx2(image_t *frame, ASS_Image *img, uint64_t offset) {
    while (img) {
        blend_single_avx2(frame, img, offset);
        img = img->next;
    }
}

} // namespace ass

#endif
