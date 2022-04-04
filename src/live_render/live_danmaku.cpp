#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <algorithm>

#include "live_danmaku.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXHttpClient.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/rapidjson/document.h"

#include "hexdump.hpp"

std::string live_danmaku::get_room_detail(int live_id) {
    using namespace ix;
    using namespace rapidjson;

    HttpClient httpClient;
    HttpRequestArgsPtr args = httpClient.createRequest();

    // Timeout options
    args->connectTimeout = 10;
    args->transferTimeout = 10;

    // Redirect options
    args->followRedirects = false;
    args->maxRedirects = 0;

    // Misc
    args->compress = false;
    args->verbose = false;
    args->logger = [](const std::string &msg) { std::cout << msg; };

    // Sync req
    HttpResponsePtr res;
    std::string url =
        std::string("https://api.live.bilibili.com/room/v1/Room/room_init?id=") +
        std::to_string(live_id);

    res = httpClient.get(url, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取房间号失败：{}\n",
                   errorMsg);
        std::abort();
    }

    auto panic_output = [&]() {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取房间号失败：{}\n",
                   body);
        std::abort();
    };

    Document doc;
    doc.Parse(body.c_str());

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        panic_output();
    }

    if (doc["code"].GetInt() != 0) {
        panic_output();
    }

    auto &data = doc["data"];
    if (!data.HasMember("room_id")) {
        panic_output();
    }

    uint64_t room_id = data["room_id"].GetInt64();

    // get random uid
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<uint64_t> uniform_dist(1, 2e14);
    uint64_t random_uid = uniform_dist(e1) + 1e14;

    return fmt::format(R"({{"roomid":{},"uid":{},"protover":2}})",
                       room_id, random_uid); // {"roomid":0000,"uid":0000,"protover":2}
}

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
void live_danmaku::run(std::string room_info) {

    const std::string heartbeat{0x00, 0x00, 0x00, 0x1f, 0x00, 0x10, 0x00, 0x01,
                                0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01,
                                0x5b, 0x6f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x20,
                                0x4f, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x5d};
    auto sz = heartbeat.size();


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
                this->process_websocket_data(msg);

        } else if (msg->type == ix::WebSocketMessageType::Open) {
            std::cout << Hexdump(start_msg_buffer.data(), start_msg_buffer.size())
                      << std::endl;
            webSocket.sendBinary(start_msg_buffer);
            std::cout << "Connection established" << std::endl;
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            // Maybe SSL is not configured properly
            std::cout << "Connection error: " << msg->errorInfo.reason << std::endl;
            std::cout << std::endl << "stop error" << std::endl;
            std::abort();
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            // Maybe SSL is not configured properly
            std::cout << std::endl << "stop close" << std::endl;
            std::abort();
        }
    });


    webSocket.start();


    //// TODO: move this
    while (1) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(25s);
        if (webSocket.getReadyState() == ix::ReadyState::Open) {
            webSocket.sendBinary(heartbeat);
        }
    }
}

void live_danmaku::process_websocket_data(const ix::WebSocketMessagePtr &msg) {
    std::string buffer = msg->str;

    std::vector<std::string> res_list;

    auto add_danmaku_res = [&](char *content, size_t data_len) {
        // hard code. Let's quickly determine the type of content
        const char *cmd = "DANMU";
        constexpr auto cmd_len = 5;

        bool is_danmaku_type =
            (content + std::min<int>(20, data_len)) !=
            std::search(content, content + std::min<int>(20, data_len), cmd, cmd + cmd_len);

        if (is_danmaku_type) {
            std::string item(content, data_len);
            item.push_back('\0'); //TODO:
            res_list.emplace_back(item);
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

    for (auto &item : res_list) {
        std::cout << item << std::endl;
    }
}

//{"cmd":"DANMU_MSG","info":
//        [[0,1,25,9920249,1648911810571,1648911121,0,"950f3045",0,0,5,"#1453BAFF,#4C2263A2,#3353BAFF",0,"{}","{}",{"mode":0,"show_player_type":0,"extra":"{\"send_from_me\":false,\"mode\":0,\"color\":9920249,\"dm_type\":0,\"font_size\":25,\"player_mode\":1,\"show_player_type\":0,\"content\":\"我要和晚晚结婚！\",\"user_hash\":\"2500800581\",\"emoticon_unique\":\"\",\"bulge_display\":0,\"direction\":0,\"pk_direction\":0,\"quartet_direction\":0,\"yeah_space_type\":\"\",\"yeah_space_url\":\"\",\"jump_to_url\":\"\",\"space_type\":\"\",\"space_url\":\"\"}"}],
//          "我要和晚晚结婚！",
//          [56327114,"聆雪听风",0,0,0,10000,1,"#00D1F1"],
//          [26,"顶碗人","向晚大魔王",22625025,398668,"",0,6809855,398668,6850801,3,1,672346917],
//          [20,0,6406234,"\u003e50000",0],
//          ["",""],
//          0,
//          3,
//          null,
//          {"ts":1648911810,"ct":"7870E0B4"},
//          0,
//          0,
//          null,
//          null,
//          0,
//          105
//        ]}
