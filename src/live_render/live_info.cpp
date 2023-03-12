#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "live_danmaku.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXHttpClient.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/rapidjson/document.h"

// noexcept
live_detail_t live_danmaku::get_room_detail(uint64_t live_id) {
    using namespace ix;
    using namespace rapidjson;

    live_detail_t live_detail;

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

        live_detail.code_ = -1;
        return live_detail;
    }

    auto error_output = [&]() {
        live_detail.code_ = -1;
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取房间号失败：{}\n",
                   body);
    };

    Document doc;
    doc.Parse(body.c_str());

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        error_output();
        return live_detail;
    }

    live_detail.code_ = doc["code"].GetInt();
    if (live_detail.code_ != 0) {
        error_output();
        return live_detail;
    }

    // get room_id

    auto &data = doc["data"];
    if (!data.HasMember("room_id")) {
        error_output();
        return live_detail;
    }

    uint64_t room_id = data["room_id"].GetInt64();
    live_detail.room_id_ = room_id;

    // get user uid

    if (!data.HasMember("uid")) {
        error_output();
        return live_detail;
    }
    live_detail.user_uid_ = data["uid"].GetInt64();

    live_detail.live_status_ = data["live_status"].GetInt();

    // get random uid
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<uint64_t> uniform_dist(1, 2e14);
    uint64_t random_uid = uniform_dist(e1) + 1e14;

    live_detail.room_detail_str_ =
        fmt::format(R"({{"roomid":{},"uid":{},"protover":2}})", room_id,
                    random_uid); // {"roomid":0000,"uid":0000,"protover":2}

    return live_detail;
}

inline std::vector<live_stream_info_t> get_live_room_stream_v1(uint64_t room_id, int qn,
                                                               std::string &proxy_address,
                                                               std::string &user_cookie,
                                                               int retry_count) {
    using namespace ix;
    using namespace rapidjson;

    std::vector<live_stream_info_t> ret;

    live_detail_t live_detail;

    HttpClient httpClient;
    HttpRequestArgsPtr args = httpClient.createRequest();

    WebSocketHttpHeaders req_header;
    if (!user_cookie.empty()) {
        req_header["cookie"] = user_cookie;
    }

    args->extraHeaders = req_header;

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
    std::string url = fmt::format(
        fmt::runtime(proxy_address + "room/v1/Room/playUrl?qn={}&cid={}&platform=web"),
        qn, room_id);

    // qn 20000 -> 4K

    res = httpClient.get(url, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        return ret;
    }

    Document doc;
    doc.Parse(body.c_str());

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        return ret;
    }

    live_detail.code_ = doc["code"].GetInt();
    if (live_detail.code_ != 0) {
        return ret;
    }

    if (!doc["data"].HasMember("durl")) {
        return ret;
    }

    bool flag = false;
    for (auto &item : doc["data"]["durl"].GetArray()) {
        std::string stream_url = item["url"].GetString();

        ret.emplace_back(live_stream_info_t::STREAM, live_stream_info_t::AVC,
                         live_stream_info_t::FLV, stream_url);
    }

    // Prefer to use gotcha05 address for better recording
    for (auto i = 0; i < ret.size(); i++) {
        auto &stream_url = ret[i].address_;
        if (stream_url.find("gotcha05") != std::string::npos) {
            fmt::print(fg(fmt::color::green_yellow), "\n获取到了gotcha05地址\n");
            std::swap(ret[0], ret[i]);
            flag = true;
            break;
        }
    }

    if (flag == false) {
        if (retry_count == 0) {
            // rollback to v2 method
            return {};
        }

        return get_live_room_stream_v1(room_id, qn, proxy_address, user_cookie,
                                       retry_count - 1);
    }

    return ret;
}

/**
 *
 * @param room_id
 * @param qn stream quality
 * @return
 */
std::vector<live_stream_info_t>
live_danmaku::get_live_room_stream(uint64_t room_id, int qn, std::string proxy_address,
                                   std::string user_cookie) {
    using namespace ix;
    using namespace rapidjson;

    int retry_count = 3;

    if (proxy_address.empty()) {
        retry_count = 0;
        proxy_address = "https://api.live.bilibili.com/";
    }

    std::vector<live_stream_info_t> ret;

    ret = get_live_room_stream_v1(room_id, qn, proxy_address, user_cookie, retry_count);
    if (!ret.empty()) {
        return ret;
    }

    live_detail_t live_detail;

    HttpClient httpClient;
    HttpRequestArgsPtr args = httpClient.createRequest();
    WebSocketHttpHeaders req_header;
    if (!user_cookie.empty()) {
        req_header["cookie"] = user_cookie;
    }

    args->extraHeaders = req_header;

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
    std::string url = fmt::format(
        fmt::runtime(proxy_address + "xlive/web-room/v2/index/"
                                     "getRoomPlayInfo?platform=web&ptype=8&qn={}&"
                                     "protocol=0,1&format=0,1,2&codec=0,1&room_id={}"),
        qn, room_id);

    // qn 20000 -> 4K

    res = httpClient.get(url, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "获取直播流信息失败：{}\n", errorMsg);
        return ret;
    }

    auto error_output = [&]() {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "获取直播流信息失败：{}\n", body);
    };

    Document doc;
    doc.Parse(body.c_str());

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        error_output();
        return ret;
    }

    live_detail.code_ = doc["code"].GetInt();
    if (live_detail.code_ != 0) {
        error_output();
        return ret;
    }

    auto &data = doc["data"];
    if (data["live_status"] != live_detail_t::VALID) {
        return ret;
    }

    auto &stream_list = data["playurl_info"]["playurl"]["stream"];

    // get max accept qn
    int max_qn = qn;
    int current_qn = qn;
    for (auto &item : stream_list.GetArray()) {
        for (auto &accpet_qn : item["format"][0]["codec"][0]["accept_qn"].GetArray()) {
            int qn_now = accpet_qn.GetInt();
            if (qn_now > max_qn) {
                max_qn = qn_now;
            }
        }
    }

    // try to get max quality stream
    if (max_qn > current_qn) {
        return get_live_room_stream(room_id, max_qn, proxy_address);
    }

    auto get_stream_address_list = [&ret](auto &item, int protocol) {
        int codec;
        int format;

        // codec : array
        auto codec_str = item["format"][0]["codec"][0]["codec_name"].GetString();

        if (codec_str == std::string("avc")) {
            codec = live_stream_info_t::AVC;
        } else if (codec_str == std::string("hevc")) {
            codec = live_stream_info_t::HEVC;
        } else if (codec_str == std::string("av1")) {
            codec = live_stream_info_t::AV1;
        } else {
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::italic, "未知的编码：{}\n",
                       codec_str);
            codec = live_stream_info_t::UNKNOWN_PROTOCOL;
        }

        auto format_str = item["format"][0]["format_name"].GetString();
        if (format_str == std::string("flv")) {
            format = live_stream_info_t::FLV;
        } else if (format_str == std::string("ts")) {
            format = live_stream_info_t::TS;
        } else if (format_str == std::string("fmp4")) {
            format = live_stream_info_t::FMP4;
        } else {
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::italic,
                       "未知的流格式：{}\n", format_str);
            format = live_stream_info_t::UNKNOWN_FORMAT;
        }

        // process url list

        auto base_url = item["format"][0]["codec"][0]["base_url"].GetString();
        auto &url_list = item["format"][0]["codec"][0]["url_info"];
        for (auto &url_info : url_list.GetArray()) {
            auto host = url_info["host"].GetString();
            auto extra = url_info["extra"].GetString();

            std::string address = fmt::format("{}{}{}", host, base_url, extra);

            ret.emplace_back(protocol, codec, format, address);
        }
    };

    // prefer to use flv

    // get flv stream
    for (auto &item : stream_list.GetArray()) {
        std::string format_name(item["format"][0]["format_name"].GetString());
        if (format_name == "flv") {
            get_stream_address_list(item, live_stream_info_t::FLV);
        }
    }

    // get ts stream
    for (auto &item : stream_list.GetArray()) {
        std::string format_name(item["format"][0]["format_name"].GetString());
        if (format_name == "ts") {
            get_stream_address_list(item, live_stream_info_t::TS);
        }
    }

    return ret;
}

std::string live_danmaku::get_live_room_title(uint64_t user_uid) {
    using namespace ix;
    using namespace rapidjson;
    using namespace fmt::literals;

    std::string ret;

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
    std::string url("https://api.live.bilibili.com/room/v1/Room/get_status_info_by_uids");

    auto req_body = fmt::format("{{\"uids\": [{user_uid}]}}", "user_uid"_a = user_uid);

    WebSocketHttpHeaders headers;
    headers["Content-Type"] = "application/json";
    args->extraHeaders = headers;

    res = httpClient.post(url, req_body, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取直播标题失败：{}\n",
                   errorMsg);
        return ret;
    }

    auto error_output = [&]() {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取直播标题失败：{}\n",
                   body);
        return ret;
    };

    Document doc;
    doc.Parse(body.c_str());

    // doc["data"][user_uid_str]["title"]

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        error_output();
        return ret;
    }

    if (doc["code"].GetInt() != 0) {
        error_output();
        return ret;
    }

    std::string uid_str = fmt::format("{}", user_uid);
    if (!doc["data"].HasMember(uid_str.c_str())) {
        error_output();
        return ret;
    }

    ret = fmt::format("{}", doc["data"][uid_str.c_str()]["title"].GetString());

    return ret;
}

std::string live_danmaku::get_username(uint64_t room_id) {
    using namespace ix;
    using namespace rapidjson;

    std::string ret;

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

    // Header
    WebSocketHttpHeaders headers;
    headers["User-Agent"] =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like "
        "Gecko) Chrome/105.0.0.0 Safari/537.36";
    args->extraHeaders = headers;

    // Sync req
    HttpResponsePtr res;
    std::string url = fmt::format("https://api.live.bilibili.com/live_user/v1/UserInfo/"
                                  "get_anchor_in_room?roomid={}",
                                  room_id);

    res = httpClient.get(url, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取用户名失败：{}\n",
                   errorMsg);
        std::abort();
    }

    auto error_output = [&]() {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "获取用户名失败：{}\n",
                   body);
    };

    Document doc;
    doc.Parse(body.c_str());

    if (!doc.HasMember("code") || !doc.HasMember("data")) {
        error_output();
        return ret;
    }

    if (doc["code"].GetInt() != 0) {
        error_output();
        return ret;
    }

    // get username

    auto &data = doc["data"];
    if (!data.HasMember("info") || !data["info"].HasMember("uname")) {
        error_output();
        return ret;
    }

    return data["info"]["uname"].GetString();
}