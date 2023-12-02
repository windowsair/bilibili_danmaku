#include <stdio.h>
#include <stdlib.h>

#include <cassert>
#include <memory>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#define POPEN  _popen
#define PCLOSE _pclose
const char *kOpenOption = "wb";
const char *kOpenReadOption = "rb";
#else
#define POPEN  popen
#define PCLOSE pclose
const char *kOpenOption = "w";
const char *kOpenReadOption = "r";
#endif


//Metadata:
//    Rawdata         :
//    displayWidth    : 1920
//    displayHeight   : 1080
//    fps             : 30
//    profile         :
//    level           :
//    encoder         : BVC-SRT LiveHime/4.14.1 (Windows)
//    Duration: N/A, start: 11856.011000, bitrate: 4358 kb/s
//    Stream #0:0: Audio: aac (LC), 48000 Hz, stereo, fltp, 262 kb/s
//    Stream #0:1: Video: h264 (High), yuv420p(tv, bt709, progressive), 1920x1080 [SAR 1:1 DAR 16:9], 4096 kb/s, 30 fps, 30 tbr, 1k tbn, 60 tbc

void verify_video_meta_info(std::string buffer) {
    std::size_t pos;
    const char *substr;

    int fps, displayWidth, displayHeight;
    int ret;

    // get displayWidth and displayHeight
    // From ffmpeg "avcodec_string()"
    pos = buffer.find("fps");
    assert(pos != std::string::npos);

    pos = buffer.find_last_of("x", pos);
    assert(pos != std::string::npos);

    // , 1920x1080
    pos = buffer.find_last_of(",", pos);
    assert(pos != std::string::npos);
    //  1920x1080
    substr = buffer.data() + pos + 1;

    ret = sscanf(substr, "%dx%d", &displayWidth, &displayHeight);
    assert(ret == 2);

    // get fps
    pos = buffer.find("fps", pos);
    assert(pos != std::string::npos);
    pos = buffer.find_last_of(",", pos);
    substr = buffer.data() + pos + 1;

    sscanf(substr, "%d", &fps);

    if (displayWidth == 1920 && displayHeight == 1080 && fps == 60) {
        printf("TEST PASS.\n");
    } else {
        printf("TEST FAILED.\n");
    }
}

int main() {
    verify_video_meta_info(R"(Stream #0:0: Video: h264 (High) (avc1 / 0x31637661), yuv420p(tv, bt709), 1920x1080 [SAR 1:1 DAR 16:9], 0 kb/s, 60 fps, 30 tbr, 90k tbn (default))");
    verify_video_meta_info(R"(Stream #0:0: Video: h264 (High), yuv420p(tv, bt470bg/bt470bg/smpte170m, progressive), 1920x1080, 10240 kb/s, 60 fps, 60 tbr, 1k tbn)");
    verify_video_meta_info(R"(Stream #0:1: Video: h264 (High) ([27][0][0][0] / 0x001B), yuv420p(tv, bt709), 1920x1080, 60 fps, 30 tbr, 90k tbn)");
    return 0;
}
