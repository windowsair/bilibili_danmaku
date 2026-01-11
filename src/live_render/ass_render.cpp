#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define ASS_TARGET_x86
#include <immintrin.h>
#include <smmintrin.h>
#endif

#include "ass_render.h"

#define RUAPU_IMPLEMENTATION
#include "thirdparty/ruapu/ruapu.h"

namespace ass {
class CpuFeatures {
  public:
    static CpuFeatures &instance() {
        static CpuFeatures inst;
        return inst;
    }

    CpuFeatures(const CpuFeatures &) = delete;
    CpuFeatures &operator=(const CpuFeatures &) = delete;

    bool has_avx2() const {
        return avx2_supported_;
    }

    bool has_ssse3() const {
        return ssse3_supported_;
    }

    bool has_sse2() const {
        return sse2_supported_;
    }

  private:
    CpuFeatures() {
        ruapu_init();
        avx2_supported_ = !!ruapu_supports("avx2");
        sse2_supported_ = !!ruapu_supports("sse2");
        ssse3_supported_ = !!ruapu_supports("ssse3");
    }

  private:
    bool sse2_supported_ = false;
    bool ssse3_supported_ = false;
    bool avx2_supported_ = false;
};

inline bool has_sse2() {
    return CpuFeatures::instance().has_sse2();
}

inline bool has_avx2() {
    return CpuFeatures::instance().has_avx2();
}

inline bool has_ssse3() {
    return CpuFeatures::instance().has_ssse3();
}

inline __m128i _mm_fast_div_255_epu16(__m128i x) {
    return _mm_srli_epi16(
        _mm_adds_epu16(x, _mm_srli_epi16(_mm_adds_epu16(x, _mm_set1_epi16(0x0101)), 8)),
        8);
}

#define ZERO_MASK ((char)0xff)

inline void blend_single_sse2(image_t *frame, ASS_Image *img, uint64_t offset) {
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

inline void ass_blend_sse2(image_t *frame, ASS_Image *img, uint64_t offset) {
    while (img) {
        blend_single_sse2(frame, img, offset);
        img = img->next;
    }
}

inline void blend_single_div255(image_t *frame, ASS_Image *img, uint64_t offset) {
    int x, y;
    unsigned char opacity = 255 - TO_A(img->color);
    unsigned char r = TO_R(img->color);
    unsigned char g = TO_G(img->color);
    unsigned char b = TO_B(img->color);

    unsigned char *src;
    unsigned char *dst;

    src = img->bitmap;
    dst = (frame->buffer + offset) + img->dst_y * frame->stride + img->dst_x * 4;
    for (y = 0; y < img->h; ++y) {
        for (x = 0; x < img->w; ++x) {
            uint32_t k = ((uint32_t)src[x]) * opacity / 255;
            // possible endianness problems...
            // would anyone actually use big endian machine??
            dst[x * 4] = (k * r + (255 - k) * dst[x * 4]) / (1 * 255);
            dst[x * 4 + 1] = (k * g + (255 - k) * dst[x * 4 + 1]) / (1 * 255);
            dst[x * 4 + 2] = (k * b + (255 - k) * dst[x * 4 + 2]) / (1 * 255);
            dst[x * 4 + 3] = (k * 255 + (255 - k) * dst[x * 4 + 3]) / (1 * 255);
        }
        src += img->stride;
        dst += frame->stride;
    }
}

inline void ass_blend_div255(image_t *frame, ASS_Image *img, uint64_t offset) {
    while (img) {
        blend_single_div255(frame, img, offset);
        img = img->next;
    }
}

void ass_blend(image_t *frame, ASS_Image *img, uint64_t offset) {
    if (false) {
    }
#ifdef ASS_TARGET_x86
    else if (has_avx2())
        ass_blend_avx2(frame, img, offset);
    else if (has_ssse3())
        ass_blend_ssse3(frame, img, offset);
    else if (has_sse2())
        ass_blend_sse2(frame, img, offset);
#endif
    else
        ass_blend_div255(frame, img, offset);
}
} // namespace ass
