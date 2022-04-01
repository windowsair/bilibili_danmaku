#ifndef BILIBILI_DANMAKU_LIVE_DANMAKU_H
#define BILIBILI_DANMAKU_LIVE_DANMAKU_H

#include <vector>

#include "thirdparty/libdeflate/libdeflate.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t packet_len;
    uint32_t magic; // don't know...
    uint32_t com_1;
    uint32_t com_2;
} live_danmaku_req_header_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint32_t packet_len;
    uint16_t header_len;
    uint16_t version;
    uint32_t op;
    uint32_t seq;
} live_danmaku_res_header_t;
#pragma pack(pop)

static_assert(sizeof(live_danmaku_res_header_t) == 16);

class live_danmaku {
  public:
    live_danmaku() {
        zlib_handle_ = libdeflate_alloc_decompressor();
        zlib_buffer_.resize(10240);
    }

    void run();

  private:
    /**
     * Decompress zlib data
     *
     * @param buffer_in [in]
     * @param buffer_in_size
     * @return decompressed data size
     *  return 0 when error occur
     */
    size_t zlib_decompress(void *buffer_in, size_t buffer_in_size);



  private:
    struct libdeflate_decompressor *zlib_handle_;
    std::vector<uint8_t> zlib_buffer_;
};

#endif //BILIBILI_DANMAKU_LIVE_DANMAKU_H
