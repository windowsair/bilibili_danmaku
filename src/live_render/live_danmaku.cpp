#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>

#include "live_danmaku.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXHttpClient.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/rapidjson/document.h"

constexpr auto user_live_render_blacklist_path = "danmaku_blacklist.txt";

size_t live_danmaku::zlib_decompress(void *buffer_in, size_t buffer_in_size) {
    size_t ret;
    auto res = libdeflate_zlib_decompress(this->zlib_handle_, buffer_in, buffer_in_size,
                                          this->zlib_buffer_.data(),
                                          this->zlib_buffer_.size(), &ret);

    if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
        // no enough space, retry
        zlib_buffer_.resize(zlib_buffer_.size() * 2);
        return zlib_decompress(buffer_in, buffer_in_size);
    } else if (res == LIBDEFLATE_BAD_DATA) {
        return 0;
    }

    return ret;
}
void live_danmaku::run(std::string room_info, config::live_render_config_t &live_config) {

    std::thread([this, room_info, &live_config]() {
        init_parser();

        ix::WebSocket webSocket;

        std::string url("wss://broadcastlv.chat.bilibili.com/sub");
        webSocket.setUrl(url);

        webSocket.setPingInterval(25);

        webSocket.disablePerMessageDeflate();

        auto packet_len = sizeof(live_danmaku_req_header_t) + room_info.size();
        live_danmaku_req_header_t start_header = {
            .packet_len = htonl(packet_len), .com_1 = htonl(7), .com_2 = htonl(1)};

        start_header.magic[0] = 0x00;
        start_header.magic[1] = 0x10;
        start_header.magic[2] = 0x00;
        start_header.magic[3] = 0x01;

        std::string start_msg_buffer;
        start_msg_buffer.resize(packet_len);
        memcpy(start_msg_buffer.data(), &start_header, sizeof(start_header));
        memcpy(start_msg_buffer.data() + sizeof(start_header), room_info.data(),
               room_info.size());

        webSocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
            if (msg->type == ix::WebSocketMessageType::Message) {

                if (msg->binary)
                    this->process_websocket_data(msg, live_config);

            } else if (msg->type == ix::WebSocketMessageType::Open) {
                webSocket.sendBinary(start_msg_buffer);
                fmt::print(fg(fmt::color::green_yellow), "与弹幕服务器建立连接\n");
            } else if (msg->type == ix::WebSocketMessageType::Error) {
                // Maybe SSL is not configured properly
                fmt::print(fg(fmt::color::crimson), "与弹幕服务器通讯失败：{}\n",
                           msg->errorInfo.reason);
            } else if (msg->type == ix::WebSocketMessageType::Close) {
                // Maybe SSL is not configured properly
                fmt::print(fg(fmt::color::crimson), "与弹幕服务器断开\n");
                // TODO: retry

                // It seems to be possible to reconnect automatically without additional configuration.
            }
        });

        webSocket.start();

        while (1) {
            using namespace std::chrono_literals;

            const std::string heartbeat{0x00, 0x00, 0x00, 0x1f, 0x00, 0x10, 0x00, 0x01,
                                        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01,
                                        0x5b, 0x6f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x20,
                                        0x4f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x5d};

            std::this_thread::sleep_for(25s);
            if (webSocket.getReadyState() == ix::ReadyState::Open) {
                webSocket.sendBinary(heartbeat);
            }
        }
    }).detach();
}

void live_danmaku::process_websocket_data(const ix::WebSocketMessagePtr &msg,
                                          config::live_render_config_t &live_config) {
    std::string buffer = msg->str;

    std::vector<std::string> danmaku_list;
    std::vector<std::string> sc_list;

    auto add_danmaku_res = [&](char *content, size_t data_len) {
        auto is_type_match = [&](const char *type_name, int type_name_len,
                                 int min_search_len = 20) {
            bool type_match =
                (content + std::min<int>(min_search_len, data_len)) !=
                std::search(content, content + std::min<int>(min_search_len, data_len),
                            type_name, type_name + type_name_len);
            return type_match;
        };

        // hard code. Let's quickly determine the type of content
        if (this->is_live_start_) [[likely]] {
            bool is_danmaku_type = is_type_match("DANMU", 5);

            if (is_danmaku_type) {
                std::string item(content, data_len);
                item.push_back('\0'); //TODO:
                danmaku_list.emplace_back(item);
                return;
            }

            if (live_config.sc_enable_) [[unlikely]] {
                bool is_sc_type = is_type_match("SUPER_CHAT_MESSAGE\"", 19,
                                                30); // ignore SUPER_CHAT_MESSAGE_JPN
                if (is_sc_type) {
                    std::string item(content, data_len);
                    item.push_back('\0'); //TODO:
                    sc_list.emplace_back(item);
                    return;
                }
            }

        } else {
            bool is_live_start_type = is_type_match(R"("cmd":"LIVE")", 12);

            if (is_live_start_type) {
                this->is_live_start_ = true;
                this->live_start_cv_.notify_all();
            }
        }
    };

    auto process_decompress_data = [&](char *decompress_data, size_t data_len) {
        while (data_len > sizeof(live_danmaku_res_header_t)) {
            auto header = reinterpret_cast<live_danmaku_res_header_t *>(decompress_data);
            auto packet_len = ntohl(header->packet_len);
            auto header_len = ntohs(header->header_len);

            if (data_len < packet_len) {
                break;
            }

            add_danmaku_res(decompress_data + header_len, packet_len - header_len);

            data_len -= packet_len;
            decompress_data += packet_len;
        }
    };

    // Note that Bilibili does not transfer packets across frames,
    // so each frame is a complete packet.

    // Sometimes incomplete frames are transmitted, which have the payload: "[object Object]".
    // Since their header is not long enough, we will just drop them.
    while (buffer.size() > sizeof(live_danmaku_res_header_t)) {
        auto header = reinterpret_cast<live_danmaku_res_header_t *>(buffer.data());

        auto packet_len = ntohl(header->packet_len);
        auto header_len = ntohs(header->header_len);
        auto version = ntohs(header->version);
        auto op = ntohl(header->op);
        //        auto seq = ntohl(header->seq);

        // now received a part of packet
        if (buffer.size() < packet_len) {
            // TODO: debug
            break;
        }

        if (packet_len == header_len) {
            // no data
            //TODO: test
            std::cout << "no data" << std::endl;
            assert(header_len == sizeof(live_danmaku_res_header_t));
            buffer = buffer.substr(packet_len);
            continue;
        }

        if (op != 5) { // danmaku type
            buffer = buffer.substr(packet_len);
            continue;
        }

        // continue to process
        char *payload_ptr = nullptr;
        size_t payload_len;

        if (version == 2) {
            size_t compress_data_size = packet_len - header_len;
            // zlib compress version
            payload_len = zlib_decompress(buffer.data() + header_len, compress_data_size);

            buffer = buffer.substr(packet_len);
            if (payload_len == 0) {
                printf("wrong payload\n");
                continue;
            }

            payload_ptr = static_cast<char *>(zlib_buffer_.data());
            process_decompress_data(payload_ptr, payload_len);

        } else if (version == 1 || version == 0) {
            // normal version
            payload_ptr = static_cast<char *>(buffer.data() + header_len);
            payload_len = packet_len - header_len;
            buffer = buffer.substr(packet_len);

            add_danmaku_res(payload_ptr, payload_len);

        } else {
            printf("\nunknown version: %d\n", version);
            // just ignore this packet
            buffer = buffer.substr(packet_len);
            continue;
        }
    }

    // parse json string, then send to ffmpeg.
    if (!danmaku_list.empty()) {
        process_danmaku_list(danmaku_list);
    }

    if (!sc_list.empty()) {
        process_sc_list(sc_list);
    }
}

void live_danmaku::init_parser() {
    const auto content_re_str = R"("content\\":\\"(.*?)\\",\\")";
    parse_helper_.content_re_ = new RE2(content_re_str);
    assert(parse_helper_.content_re_->ok());

    const auto danmaku_type_re_str = R"(dm_type\\":(\d*))";
    parse_helper_.danmaku_type_re_ = new RE2(danmaku_type_re_str);
    assert(parse_helper_.danmaku_type_re_->ok());

    const auto danmaku_info_re_str = R"("info":[[][[].*?,(\d),.*?,.*?,(\d*))";
    // [_, danmaku_player_type, font_size, _, timestamp]
    parse_helper_.danmaku_info_re_ = new RE2(danmaku_info_re_str);
    assert(parse_helper_.danmaku_info_re_->ok());

    const auto danmaku_color_re_str = R"(\\"color\\":(\d*))";
    parse_helper_.danmaku_color_re_ = new RE2(danmaku_color_re_str);
    assert(parse_helper_.danmaku_color_re_->ok());

    // vertical type danmaku include sub str   \\r
    const auto danmaku_vertical_cr_re_str = R"(\\\\r)";
    parse_helper_.danmaku_vertical_cr_re_ = new RE2(danmaku_vertical_cr_re_str);
    assert(parse_helper_.danmaku_vertical_cr_re_->ok());

    const auto sc_content_re_str = R"(message\":\"(.*?)\",\")";
    parse_helper_.sc_content_re_ = new RE2(sc_content_re_str);
    assert(parse_helper_.sc_content_re_->ok());

    const auto sc_user_name_re_str = R"(uname\":\"(.*?)\",\")";
    parse_helper_.sc_user_name_re_ = new RE2(sc_user_name_re_str);
    assert(parse_helper_.sc_user_name_re_->ok());

    const auto sc_price_re_str = R"(price\":(\d*))";
    parse_helper_.sc_price_re_ = new RE2(sc_price_re_str);
    assert(parse_helper_.sc_price_re_->ok());

    const auto sc_start_time_re_str = R"(ts\":(\d*))";
    parse_helper_.sc_start_time_re_ = new RE2(sc_start_time_re_str);
    assert(parse_helper_.sc_start_time_re_->ok());
}

// FIXME: trim raw content
void live_danmaku::process_danmaku_list(std::vector<std::string> &raw_danmaku) {
    int color;
    int danmaku_origin_type;
    int danmaku_player_type;
    uint64_t timestamp;
    std::string content;
    float start_time;

    std::vector<danmaku::danmaku_item_t> danmaku_list;

    if (base_time_ == 0) {
        // Use the manual time as the base time.
        return;

        // find start base time
        //        uint64_t min_timestamp = UINT64_MAX;
        //
        //        for (auto &item : raw_danmaku) {
        //            RE2::PartialMatch(item, *(parse_helper_.danmaku_info_re_),
        //                              &danmaku_player_type, &timestamp);
        //
        //            min_timestamp = std::min<uint64_t>(min_timestamp, timestamp);
        //        }
        //
        //        base_time_ = min_timestamp - 10; // 10ms offset
    }

    for (auto &item : raw_danmaku) {
        RE2::PartialMatch(item, *(parse_helper_.danmaku_info_re_), &danmaku_player_type,
                          &timestamp);

        // ignore pos type danmaku
        if (danmaku_player_type != static_cast<int>(danmaku::danmaku_type::MOVE) &&
            !is_pos_danmaku_process_) {
            continue;
        }

        RE2::PartialMatch(item, *(parse_helper_.content_re_), &content);
        RE2::PartialMatch(item, *(parse_helper_.danmaku_type_re_), &danmaku_origin_type);

        RE2::PartialMatch(item, *(parse_helper_.danmaku_color_re_), &color);

        start_time = (float)(timestamp - base_time_) / (float)1000.0f;

        if (!danmaku_item_pre_process(color, danmaku_origin_type, danmaku_player_type,
                                      timestamp, start_time, content)) {
            continue; // this item should be dropped
        }

        constexpr int font_size = 25;
        danmaku_list.emplace_back(content, start_time, danmaku_player_type, font_size,
                                  color);
    }

    assert(danmaku_queue_ != nullptr);

    danmaku_recv_count_ += danmaku_list.size();

    if (!do_not_print_danmaku_info_) {
        fmt::print("总弹幕数:{}\n", danmaku_recv_count_);
    }

    if (!danmaku_list.empty()) {
        danmaku_queue_->enqueue(std::move(danmaku_list));
    }
}

bool live_danmaku::danmaku_item_pre_process(int &color, int &danmaku_origin_type,
                                            int &danmaku_player_type, uint64_t &timestamp,
                                            float &start_time, std::string &content) {
    // 1. process vertical danmaku
    bool ret = true;

    switch (this->vertical_danmaku_process_strategy_) {
    case config::verticalProcessEnum::DEFAULT:
        break;
    case config::verticalProcessEnum::DROP:
        if (RE2::PartialMatch(content, *(parse_helper_.danmaku_vertical_cr_re_))) {
            ret &= false;
        }
        break;
    case config::verticalProcessEnum::CONVERT:
        RE2::GlobalReplace(&content, *(parse_helper_.danmaku_vertical_cr_re_), "");
        break;
    default:
        break;
    }

    // add more custom rules here...

    // 2. check blacklist
    if (this->is_blacklist_used_) {
        for (auto re2_obj : this->blacklist_regex_) {
            if (RE2::PartialMatch(content, *(re2_obj))) {
                return false;
            }
        }
    }

    // 3. drop history danmaku
    if (timestamp < this->base_time_) {
        ret &= false;
    }

    return ret;
}

void live_danmaku::init_blacklist() {
    std::filesystem::path file_path(user_live_render_blacklist_path);

    if (!std::filesystem::exists(file_path)) {
        this->is_blacklist_used_ = false;
        return;
    } else {
        this->is_blacklist_used_ = true;
    }

    std::string filename(user_live_render_blacklist_path);
    std::vector<std::string> regex_lines;
    std::string line;

    std::ifstream input_file(filename);
    if (!input_file.is_open()) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "无法读取弹幕黑名单文件");
        std::abort();
    }

    while (getline(input_file, line)) {
        regex_lines.push_back(line);
    }

    input_file.close();

    for (auto i = 0; i < regex_lines.size(); i++) {
        auto &item = regex_lines[i];
        if (item.empty()) {
            continue;
        }

        RE2 *p = new RE2(item);
        if (p == nullptr || !p->ok()) {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                       "弹幕黑名单第{}行无效:{}", i, item);
            std::abort();
        }

        this->blacklist_regex_.push_back(p);
    }

    fmt::print(fg(fmt::color::green_yellow), "已启用弹幕黑名单，共{}条规则\n",
               this->blacklist_regex_.size());
}

void live_danmaku::process_sc_list(std::vector<std::string> &raw_sc) {
    std::string user_name, sc_content;
    int price;
    uint64_t start_time;

    std::vector<sc::sc_item_t> sc_list;

    for (auto &item : raw_sc) {
        RE2::PartialMatch(item, *(parse_helper_.sc_content_re_), &sc_content);
        RE2::PartialMatch(item, *(parse_helper_.sc_user_name_re_), &user_name);
        RE2::PartialMatch(item, *(parse_helper_.sc_price_re_), &price);
        RE2::PartialMatch(item, *(parse_helper_.sc_start_time_re_), &start_time);
        sc_list.emplace_back(user_name, sc_content, start_time, price);
    }

    if (!sc_list.empty() && sc_queue_) {
        sc_queue_->enqueue(std::move(sc_list));
    }
}