#ifndef BILIBILI_DANMAKU_LIVE_DANMAKU_H
#define BILIBILI_DANMAKU_LIVE_DANMAKU_H

#include <vector>

#include "danmaku.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXNetSystem.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXUserAgent.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXWebSocket.h"
#include "thirdparty/libdeflate/libdeflate.h"
#include "thirdparty/re2/re2/re2.h"
#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

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


typedef struct live_stream_info {
    int video_height_;
    int video_width_;
    int fps_;
} live_stream_info_t;

typedef struct live_detail {
    int code_;
    enum live_status_enum { VALID = 1, INVALID };
    int live_status_;
    int room_id_;
    std::string room_detail_str_;
    std::string title_;
    live_detail() : code_(-1), live_status_(INVALID) {
    }
} live_detail_t;

class live_danmaku {
  public:
    live_danmaku() : base_time_(0) {
        zlib_handle_ = libdeflate_alloc_decompressor();
        zlib_buffer_.resize(10240);

        ix::initNetSystem();
    }
    ~live_danmaku() {
        libdeflate_free_decompressor(zlib_handle_);
        delete parse_helper_.content_re_;
        delete parse_helper_.danmaku_type_re_;
        delete parse_helper_.danmaku_color_re_;
        delete parse_helper_.danmaku_info_re_;
    }
    live_detail_t get_room_detail(int live_id);

    std::vector<std::string> get_live_room_stream(int room_id, int qn);

    live_stream_info_t get_live_stream_info(std::string& stream_address);

    void run(std::string room_info);

    void set_danmaku_queue(
        moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *p) {
        danmaku_queue_ = p;
    }

  private:
    void init_parser();

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

    /**
     *
     * @param danmaku_list
     */
    void process_danmaku_list(std::vector<std::string> &raw_danmaku);

  private:
    struct libdeflate_decompressor *zlib_handle_;
    std::vector<char> zlib_buffer_;
    class ParseHelper {
      public:
        RE2 *content_re_;
        RE2 *danmaku_type_re_;
        RE2 *danmaku_color_re_;
        RE2 *danmaku_info_re_;
    } parse_helper_;
    uint64_t base_time_;
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *danmaku_queue_;
};

#endif //BILIBILI_DANMAKU_LIVE_DANMAKU_H
