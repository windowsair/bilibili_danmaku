#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <array>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "live_danmaku.h"

#include "thirdparty/IXWebSocket/ixwebsocket/IXHttpClient.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/rapidjson/document.h"
#include <openssl/md5.h>

class bili_wbi {
  private:
    std::string key_;

    constexpr static std::array<uint8_t, 64> MIXIN_KEY_ENC_TAB_ = {
        46, 47, 18, 2,  53, 8,  23, 32, 15, 50, 10, 31, 58, 3,  45, 35,
        27, 43, 5,  49, 33, 9,  42, 19, 29, 28, 14, 39, 12, 38, 41, 13,
        37, 48, 7,  16, 24, 55, 40, 61, 26, 17, 0,  1,  60, 51, 30, 4,
        22, 25, 54, 21, 56, 59, 6,  63, 57, 62, 11, 36, 20, 34, 44, 52};

    std::string get_key(const std::string &input_key) {
        std::string ret;

        for (uint8_t x : MIXIN_KEY_ENC_TAB_) {
            ret.push_back(input_key[x]);
        }

        return ret.substr(0, 32);
    }

    std::string get_md5_hex(const std::string &input_str) {
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5_CTX md5;
        MD5_Init(&md5);
        MD5_Update(&md5, input_str.c_str(), input_str.size());
        MD5_Final(hash, &md5);

        std::string out;
        out.reserve(MD5_DIGEST_LENGTH * 2);

        for (unsigned char x : hash) {
            out += fmt::format("{:02x}", x);
        }

        return out;
    }

    std::string fetch_bili_key(std::string &user_cookie) {
        using namespace ix;
        using namespace rapidjson;

        std::string ret;
        HttpClient httpClient;
        HttpRequestArgsPtr args = httpClient.createRequest();
        WebSocketHttpHeaders req_header;
        if (!user_cookie.empty()) {
            req_header["cookie"] = user_cookie;
        }

        req_header["User-Agent"] =
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like "
            "Gecko) Chrome/105.0.0.0 Safari/537.36";

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
        std::string url = "https://api.bilibili.com/x/web-interface/nav";

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

        if (doc["code"].GetInt() != 0) {
            error_output();
            return ret;
        }

        if (!doc["data"].HasMember("wbi_img")) {
            error_output();
            return ret;
        }

        auto &wbi_img = doc["data"]["wbi_img"];
        if (!wbi_img.HasMember("img_url") || !wbi_img.HasMember("sub_url")) {
            error_output;
            return ret;
        }

        const std::string img_url = wbi_img["img_url"].GetString();
        const std::string sub_url = wbi_img["sub_url"].GetString();

        std::string img_key = img_url.substr(
            img_url.find("wbi/") + 4, img_url.find(".png") - img_url.find("wbi/") - 4);
        std::string sub_key = sub_url.substr(
            sub_url.find("wbi/") + 4, sub_url.find(".png") - sub_url.find("wbi/") - 4);

        return img_key + sub_key;
    }

  public:
    bili_wbi(std::string user_cookie) {
        std::string ret;

        ret = fetch_bili_key(user_cookie);
        if (ret.empty()) {
            return;
        }

        key_ = get_key(ret);
    }

    // request with wts timestamp
    std::string get_rid(std::string str) {
        std::string ret;
        std::string w_rid;
        std::string md5_input;
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();

        // "str&wts=xxx" + key_ -> MD5 -> w_rid
        ret = str + fmt::format("&wts={}", timestamp);
        md5_input = ret;
        md5_input.append(key_);
        w_rid = get_md5_hex(md5_input);

        return ret + fmt::format("&w_rid={}", w_rid);
    }
};

void bili_wbi_delete(bili_wbi *ctx) {
    if (ctx) {
        delete ctx;
    }
}

inline std::unordered_map<std::string, std::string> parse_cookie(std::string_view cookie) {
	std::unordered_map<std::string, std::string> kv;
	size_t pos = 0, end = cookie.size();

	auto trim = [](std::string const& s) -> std::string {
		std::string_view sv = s;
		auto first = sv.find_first_not_of(" ");
		if (first == sv.npos)
			return {};
		auto last = sv.find_last_not_of(" ");
		return std::string(sv.substr(first, last - first + 1));
	};

	while (pos < end) {
		while (pos < end && (cookie[pos] == ' ' || cookie[pos] == ';')) {
			++pos;
		}

		if (pos >= end) {
			break;
		}

		size_t eq = cookie.find('=', pos);
		if (eq == std::string_view::npos) {
			break;
		}

		std::string name = std::string(cookie.substr(pos, eq - pos));
		name = trim(name);

		size_t valEnd = cookie.find_first_of("; ", eq + 1);
		if (valEnd == std::string_view::npos) {
			valEnd = end;
		}
		std::string value = std::string(cookie.substr(eq + 1, valEnd - eq - 1));

		if (!value.empty() && value.back() == ' ') {
			value.pop_back();
		}

		kv[std::move(name)] = std::move(value);
		pos = valEnd;
	}

	return kv;
}

inline std::string get_danmaku_ws_token(uint64_t room_id, const std::string &user_cookie,
                                        bili_wbi *ctx) {
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

    WebSocketHttpHeaders headers;
    headers["User-Agent"] =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like "
        "Gecko) Chrome/105.0.0.0 Safari/537.36";
    headers["origin"] = "https://live.bilibili.com";
    headers["referer"] = "https://live.bilibili.com/";

    auto cookie_list = parse_cookie(user_cookie);
    // When uid is 0, only "buvid3" and "buvid4" should be used
    auto cookie = fmt::format("buvid3={};buvid4={};", cookie_list["buvid3"], cookie_list["buvid4"]);
    headers["Cookie"] = cookie;
    args->extraHeaders = headers;

    std::string url_parameter = fmt::format("id={}&type=0&web_location=444.8", room_id);

    url_parameter = ctx->get_rid(url_parameter);
    std::string url =
        "https://api.live.bilibili.com/xlive/web-room/v1/index/getDanmuInfo?" +
        url_parameter;

    // Sync req
    HttpResponsePtr res;
    res = httpClient.get(url, args);

    auto statusCode = res->statusCode;
    auto errorCode = res->errorCode;
    auto responseHeaders = res->headers;
    auto body = res->body;
    auto errorMsg = res->errorMsg;

    auto error_output = [&]() {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "获取弹幕服务器Token失败：{}\n", body);
    };

    if (errorCode != HttpErrorCode::Ok || statusCode != 200) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "获取弹幕服务器Token失败：{}\n", errorMsg);
        return ret;
    }

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

    if (!doc["data"].HasMember("token")) {
        error_output();
        return ret;
    }

    ret = doc["data"]["token"].GetString();
    return ret;
}

// noexcept
live_detail_t live_danmaku::get_room_detail(uint64_t live_id) {
    using namespace ix;
    using namespace rapidjson;

    if (wbi_ctx_ == nullptr) {
        wbi_ctx_ = std::unique_ptr<bili_wbi, decltype(&bili_wbi_delete)>(
            new bili_wbi(this->user_cookie_), bili_wbi_delete);
    }

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

    std::string ws_token =
        get_danmaku_ws_token(room_id, this->user_cookie_, this->wbi_ctx_.get());
    if (ws_token.empty()) {
        return live_detail;
    }

    // uid 0 for guest user
    live_detail.room_detail_str_ = fmt::format(
        R"({{"roomid":{},"uid":{},"key":"{}","type":2,"protover":3,"platform":"web"}})",
        room_id, 0, ws_token);

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

    // gotcha address doesn't seem to work anymore

    //ret = get_live_room_stream_v1(room_id, qn, proxy_address, user_cookie, retry_count);
    //if (!ret.empty()) {
    //    return ret;
    //}

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
        fmt::runtime(
            proxy_address +
            "xlive/web-room/v2/index/"
            "getRoomPlayInfo?platform=web&ptype=8&qn={}&"
            "protocol=0,1&format=0,1,2&codec=0&ptype=8&dolby=5&panorama=1&room_id={}"),
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
    bool fail_to_get_max_qn = false;
    for (auto &item : stream_list.GetArray()) {
        int min_qn = INT32_MAX;
        int qn_count = 0;
        for (auto &accpet_qn : item["format"][0]["codec"][0]["accept_qn"].GetArray()) {
            int qn_now = accpet_qn.GetInt();

            if (qn_now > max_qn) {
                max_qn = qn_now;
            }

            if (qn_now < min_qn) {
                min_qn = qn_now;
            }

            qn_count++;
        }

        int accurary_qn = item["format"][0]["codec"][0]["current_qn"].GetInt();
        if (qn_count > 1 && accurary_qn == min_qn) {
            fail_to_get_max_qn = true;
        }
    }

    if (fail_to_get_max_qn) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "无法获取最高画质直播流，可能是Cookie信息失效\n");
    }

    // try to get max quality stream
    if (max_qn > current_qn) {
        return get_live_room_stream(room_id, max_qn, proxy_address);
    }

    auto get_stream_address_list = [&ret](auto &item, int protocol) {
        int codec;
        int format;

        // codec : array
        auto codec_str = item["codec"][0]["codec_name"].GetString();

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

        auto format_str = item["format_name"].GetString();
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
        auto base_url = item["codec"][0]["base_url"].GetString();
        auto &url_list = item["codec"][0]["url_info"];
        for (auto &url_info : url_list.GetArray()) {
            auto host = url_info["host"].GetString();
            auto extra = url_info["extra"].GetString();

            std::string address = fmt::format("{}{}{}", host, base_url, extra);

            ret.emplace_back(protocol, codec, format, address);
        }
    };

    for (auto &stream_item : stream_list.GetArray()) {
        for (auto &format_item : stream_item["format"].GetArray()) {
            std::string format_name(format_item["format_name"].GetString());
            if (format_name == "flv") {
                get_stream_address_list(format_item, live_stream_info_t::FLV);
            } else if (format_name == "ts") {
                get_stream_address_list(format_item, live_stream_info_t::TS);
            } else if (format_name == "fmp4") {
                get_stream_address_list(format_item, live_stream_info_t::FMP4);
            }
        }
    }

    // prefer to use flv
    std::sort(ret.begin(), ret.end(),
              [](const live_stream_info_t &a, const live_stream_info_t &b) {
                  return a.format_ < b.format_;
              });

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