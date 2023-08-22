#ifndef BILIBILI_DANMAKU_LIVE_DANMAKU_H
#define BILIBILI_DANMAKU_LIVE_DANMAKU_H

#include <condition_variable>
#include <mutex>
#include <vector>

#include "danmaku.h"
#include "live_render_config.h"
#include "sc_item.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXNetSystem.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXUserAgent.h"
#include "thirdparty/IXWebSocket/ixwebsocket/IXWebSocket.h"
#include "thirdparty/libdeflate/libdeflate.h"
#include "thirdparty/re2/re2/re2.h"
#include "thirdparty/readerwriterqueue/readerwriterqueue.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t packet_len;
    uint8_t magic[4]; // don't know... maybe header_len?
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
    enum protocol_enum { STREAM = 0, HLS, UNKNOWN_PROTOCOL };
    enum codec_enum { AVC = 0, AV1, HEVC, UNKNOWN_CODEC };
    enum format_enum { FLV = 0, TS, FMP4, UNKNOWN_FORMAT };

    int protocol_;
    int codec_;
    int format_;

    std::string address_;

    live_stream_info(int protocol, int codec, int format, std::string address)
        : protocol_(protocol), codec_(codec), format_(format), address_(address) {
    }

} live_stream_info_t;

typedef struct live_stream_video_info {
    int video_height_;
    int video_width_;
    int fps_;
} live_stream_video_info_t;

typedef struct live_detail {
    int code_; // 0: OK, -1: Req failed
    enum live_status_enum { VALID = 1, INVALID };
    int live_status_;
    int room_id_;
    std::string room_detail_str_;
    uint64_t user_uid_;
    live_detail() : code_(-1), live_status_(INVALID) {
    }
} live_detail_t;

class live_danmaku {
  public:
    live_danmaku()
        : base_time_(0), danmaku_recv_count_(0), is_pos_danmaku_process_(false),
          do_not_print_danmaku_info_(false), is_live_start_(false),
          danmaku_queue_(nullptr), sc_queue_(nullptr) {
        zlib_handle_ = libdeflate_alloc_decompressor();
        zlib_buffer_.resize(10240);

        parse_helper_.content_re_ = nullptr;
        parse_helper_.danmaku_type_re_ = nullptr;
        parse_helper_.danmaku_color_re_ = nullptr;
        parse_helper_.danmaku_info_re_ = nullptr;
        parse_helper_.danmaku_vertical_cr_re_ = nullptr;

        parse_helper_.sc_content_re_ = nullptr;
        parse_helper_.sc_user_name_re_ = nullptr;
        parse_helper_.sc_price_re_ = nullptr;
        parse_helper_.sc_start_time_re_ = nullptr;

        ix::initNetSystem();
    }
    ~live_danmaku() {
        libdeflate_free_decompressor(zlib_handle_);
        delete parse_helper_.content_re_;
        delete parse_helper_.danmaku_type_re_;
        delete parse_helper_.danmaku_color_re_;
        delete parse_helper_.danmaku_info_re_;
        delete parse_helper_.danmaku_vertical_cr_re_;
        delete parse_helper_.sc_content_re_;
        delete parse_helper_.sc_user_name_re_;
        delete parse_helper_.sc_price_re_;

        for (auto item : blacklist_regex_) {
            delete item;
        }
    }

  public:
    std::mutex live_start_mutex_;
    std::condition_variable live_start_cv_;

  public:
    live_detail_t get_room_detail(uint64_t live_id);

    std::vector<live_stream_info_t> get_live_room_stream(uint64_t room_id, int qn,
                                                         std::string proxy_address = "",
                                                         std::string user_cookie = "");

    std::string get_live_room_title(uint64_t user_uid);

    std::string get_username(uint64_t room_id);

    void init_blacklist();

    void run(std::string room_info, config::live_render_config_t &live_config);

    bool is_live_start() const {
        return is_live_start_;
    }

    void set_live_start_status(bool is_live_start) {
        is_live_start_ = is_live_start;
    }

    void update_base_time(uint64_t base_time_in_ms) {
        base_time_ = base_time_in_ms;
    }

    void set_danmaku_queue(
        moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *p) {
        danmaku_queue_ = p;
    }

    void set_sc_queue(moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> *p) {
        sc_queue_ = p;
    }

    void enable_pos_danmaku_process() {
        is_pos_danmaku_process_ = true;
    }

    void disable_pos_danmaku_process() {
        is_pos_danmaku_process_ = false;
    }

    void enable_danmaku_stat_info() {
        do_not_print_danmaku_info_ = false;
    }

    void disable_danmaku_stat_info() {
        do_not_print_danmaku_info_ = true;
    }

    void set_vertical_danmaku_process_strategy(int strategy_num) {
        vertical_danmaku_process_strategy_ = strategy_num;
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

    void process_websocket_data(const ix::WebSocketMessagePtr &msg,
                                config::live_render_config_t &live_config);

    /**
     *
     * @param danmaku_list
     */
    void process_danmaku_list(std::vector<std::string> &raw_danmaku);

    void process_sc_list(std::vector<std::string> &raw_sc);

    /**
     * Customize danmaku item.
     *
     * @param color
     * @param danmaku_origin_type
     * @param danmaku_player_type
     * @param timestamp
     * @param start_time
     * @param content
     * @return true:This danmaku is acceptable. false: This danmaku should be dropped.
     */
    bool danmaku_item_pre_process(int &color, int &danmaku_origin_type,
                                  int &danmaku_player_type, uint64_t &timestamp,
                                  float &start_time, std::string &content);

  private:
    struct libdeflate_decompressor *zlib_handle_;
    std::vector<char> zlib_buffer_;
    class ParseHelper {
      public:
        // danmaku type
        RE2 *content_re_;
        RE2 *danmaku_type_re_;
        RE2 *danmaku_color_re_;
        RE2 *danmaku_info_re_;
        RE2 *danmaku_vertical_cr_re_;

        // sc type
        RE2 *sc_content_re_;
        RE2 *sc_user_name_re_;
        RE2 *sc_price_re_;
        RE2 *sc_start_time_re_;
    } parse_helper_;
    uint64_t base_time_;
    moodycamel::ReaderWriterQueue<std::vector<danmaku::danmaku_item_t>> *danmaku_queue_;
    moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> *sc_queue_;
    int danmaku_recv_count_;
    int vertical_danmaku_process_strategy_; // config::verticalProcessEnum
    bool is_pos_danmaku_process_;
    bool do_not_print_danmaku_info_;

    // live status
    bool is_live_start_;

    bool is_blacklist_used_;
    std::vector<RE2 *> blacklist_regex_;
};

#endif //BILIBILI_DANMAKU_LIVE_DANMAKU_H
