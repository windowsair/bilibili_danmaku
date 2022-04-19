#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>

#include "thirdparty/libass/include/ass.h"

#include <chrono>

#if defined(_WIN32) || defined(_WIN64)
#define POPEN  _popen
#define PCLOSE _pclose
const char *kOpenOption = "wb";
#else
#define POPEN  popen
#define PCLOSE pclose
const char *kOpenOption = "w";
#endif

extern "C" {
int ass_process_events_line(ASS_Track *track, char *str);
}

ASS_Library *ass_library;
ASS_Renderer *ass_renderer;

typedef struct image_s {
    int width, height, stride;
    unsigned char *buffer; // RGB24
} image_t;

void msg_callback(int level, const char *fmt, va_list va, void *data) {
    if (level > 6)
        return;
    printf("libass: ");
    vprintf(fmt, va);
    printf("\n");
}

#define TO_R(c) ((c) >> 24)
#define TO_G(c) (((c) >> 16) & 0xFF)
#define TO_B(c) (((c) >> 8) & 0xFF)
#define TO_A(c) ((c)&0xFF)

inline void blend_single(image_t *frame, ASS_Image *img) {
    int x, y;
    unsigned char opacity = 255 - TO_A(img->color);
    unsigned char r = TO_R(img->color);
    unsigned char g = TO_G(img->color);
    unsigned char b = TO_B(img->color);

    unsigned char *src;
    unsigned char *dst;

    src = img->bitmap;
    dst = frame->buffer + img->dst_y * frame->stride + img->dst_x * 4;
    for (y = 0; y < img->h; ++y) {
        for (x = 0; x < img->w; ++x) {
            unsigned k = ((unsigned)src[x]) * opacity / 255;
            // possible endianness problems
            dst[x * 4] = (k * r + (255 - k) * dst[x * 4]) / 255;
            dst[x * 4 + 1] = (k * g + (255 - k) * dst[x * 4 + 1]) / 255;
            dst[x * 4 + 2] = (k * b + (255 - k) * dst[x * 4 + 2]) / 255;
            dst[x * 4 + 3] = (k * 255 + (255 - k) * dst[x * 4 + 3]) / 255;
        }
        src += img->stride;
        dst += frame->stride;
    }
}

inline void blend(image_t *frame, ASS_Image *img) {
    int cnt = 0;
    while (img) {
        blend_single(frame, img);
        ++cnt;
        img = img->next;
    }
    //printf("%d images blended\n", cnt);
}

inline image_t *gen_image(int width, int height) {
    image_t *img = (image_t *)malloc(sizeof(image_t));
    img->width = width;
    img->height = height;
    img->stride = width * 4;
    img->buffer = (unsigned char *)calloc(1, height * width * 4);
    memset(img->buffer, 63, img->stride * img->height);
    //for (int i = 0; i < height * width * 3; ++i)
    // img->buffer[i] = (i/3/50) % 100;
    return img;
}

inline void init(int frame_w, int frame_h) {
    ass_library = ass_library_init();
    if (!ass_library) {
        printf("ass_library_init failed!\n");
        exit(1);
    }

    ass_set_message_cb(ass_library, msg_callback, NULL);
    ass_set_extract_fonts(ass_library, 1);

    ass_renderer = ass_renderer_init(ass_library);
    if (!ass_renderer) {
        printf("ass_renderer_init failed!\n");
        exit(1);
    }

    ass_set_frame_size(ass_renderer, frame_w, frame_h);
    ass_set_fonts(ass_renderer, NULL, "sans-serif", ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
}

int main() {

    init(1920, 1080);

    ASS_Track *track = ass_read_file(ass_library, (char *)"D:/out.ass", NULL);

    int tm = 0;

    char cmd[] = "K:/ff/ffmpeg.exe -y  -vsync 0   -hwaccel nvdec -hwaccel_device 0   -i "
                 "K:/ff/1.flv -f rawvideo -s 1920x1080 -pix_fmt rgba -r 60 -i - "
                 "-filter_complex [0:v][1:v]overlay=0:0[v] -map \"[v]\"  -map  \"0:a\"   "
                 "-c:v:0 h264_nvenc -b:v:0 5650k -f mp4 \"my_test1.mp4\"";

    FILE *ffmpeg_ = POPEN(cmd, kOpenOption);
    if (ffmpeg_ == NULL) {
        fprintf(stderr, "OpenGLVideoRecorder() => failed to open ffmpeg command");
        exit(0);
    }
    image_t *frame = gen_image(1920, 1080);
    //ass_set_cache_limits(ass_renderer, 0, 50);

    while (tm < 1000 * 60 * 20) {

        ASS_Image *img = ass_render_frame(ass_renderer, track, tm, NULL);
        // clear buffer
        memset(frame->buffer, 0, 1920 * 1080 * 4);
        blend(frame, img);

        if (tm % 800 == 0)
            printf("%d\n", tm);
        tm += ((double)(1000) / (double)(60));

        auto sz = fwrite(frame->buffer, 1, 1920 * 1080 * 4, ffmpeg_);

        //free(frame->buffer);
        //free(frame);
    }

    memset(frame->buffer, 0, 1920 * 1080 * 4);

    auto sz = fwrite(frame->buffer, 1, 1920 * 1080 * 4, ffmpeg_);

    return 0;
}