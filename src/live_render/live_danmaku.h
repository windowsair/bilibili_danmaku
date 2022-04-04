#ifndef BILIBILI_DANMAKU_LIVE_DANMAKU_H
#define BILIBILI_DANMAKU_LIVE_DANMAKU_H

#include <vector>

#include "thirdparty/libdeflate/libdeflate.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXNetSystem.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXUserAgent.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXWebSocket.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t packet_len;
    uint8_t magic[4]; // don't know...
    uint32_t com_1;
    uint32_t com_2;
} live_danmaku_req_header_t;
#pragma pack(pop)

static_assert(sizeof(live_danmaku_req_header_t) == 16);

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
        ix::initNetSystem();
    }
    ~live_danmaku() {
        libdeflate_free_decompressor(zlib_handle_);
    }
    std::string get_room_detail(int live_id);

    void run(std::string room_info);

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
    void process_websocket_data(const ix::WebSocketMessagePtr &msg);


  private:
    struct libdeflate_decompressor *zlib_handle_;
    std::vector<char> zlib_buffer_;
};

#endif //BILIBILI_DANMAKU_LIVE_DANMAKU_H
