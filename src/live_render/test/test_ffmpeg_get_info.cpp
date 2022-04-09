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
int main() {
    std::string buffer;
    buffer.resize(10240);

    char cmd[] = "K:/ff/ffmpeg.exe -i "
                 "K:/ff/1.flv 2>&1"; // stderr > stdout

    // TODO: close pipe
    FILE *ffmpeg_ = POPEN(cmd, kOpenReadOption);
    if (ffmpeg_ == NULL) {
        fprintf(stderr, "failed to open ffmpeg command");
        exit(0);
    }

    int displayWidth = -1, displayHeight = -1, fps = -1;

    auto get_item = [&](auto index, int &item) {
        if (index != std::string::npos) {
            auto start_index = strstr(buffer.data() + index, ":");
            assert(start_index != nullptr);
            sscanf(start_index, ":%d", &item);
        }
    };

    while (!feof(ffmpeg_)) {
        if (fgets(buffer.data(), 10240 - 1, ffmpeg_) != NULL) {
            auto displayWidth_it = buffer.find("displayWidth");
            auto displayHeight_it = buffer.find("displayHeight");
            auto fps_it = buffer.find("fps");

            get_item(displayWidth_it, displayWidth);
            get_item(displayHeight_it, displayHeight);
            get_item(fps_it, fps);

            if (displayWidth != -1 && displayHeight != -1 && fps != -1) {
                break;
            }
        }
    }

    if (displayWidth != -1 && displayHeight != -1 && fps != -1) {
        printf("TEST PASS.\n");
    } else {
        printf("TEST FAILED.\n");
    }

    return 0;
}
