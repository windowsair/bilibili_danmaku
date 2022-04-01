#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "config.h"
#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"
#include "thirdparty/fmt/include/fmt/os.h"

#include "thirdparty/rapidjson/document.h"
#include "thirdparty/rapidjson/filereadstream.h"
#include "thirdparty/rapidjson/schema.h"

#include "config_template.hpp"

#include <cstdio>
#include <filesystem>
#include <vector>

namespace config {

constexpr auto user_config_path = "config.json";

inline ass_config_t get_default_config() {
    ass_config_t config = {
        .video_width_ = 1920,
        .video_height_ = 1080,
        .font_family_ = "微软雅黑",
        .font_color_ = 0xFFFFFF, // white
        .font_size_ = 1,
        .font_scale_ = 1.6f,
        .font_alpha_ = 0.75f,
        .font_bold_ = true,
        .danmaku_show_range_ = 0.45f,
        .danmaku_move_time_ = 15,
        .danmaku_pos_time_ = 5,
    };

    return config;
}

inline void generate_default_config() {

    auto out = fmt::output_file(user_config_path);
    out.print("{}", config_template_json);

    out.flush();
    out.close();
}

/**
 * Get the user configuration, or create it if it does not exist.
 * If the user configuration is invalid, use the default configuration.
 * @return ass config
 */
ass_config_t get_user_config() {
    std::filesystem::path file_path(user_config_path);

    if (!std::filesystem::exists(file_path)) {
        // not exist, then generate
        generate_default_config();
        return get_default_config();
    }

    // get user config
    using namespace rapidjson;

    std::vector<char> buffer(65536);

    FILE *fp = fopen(user_config_path, "rb");
    FileReadStream is(fp, buffer.data(), buffer.size());

    Document doc;
    doc.ParseStream(is);

    // Schema verify
    Document origin_schema_doc;
    if (origin_schema_doc.Parse(config_template_schema).HasParseError()) {
        // should not be happen...
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "内部错误：Schema无效\n");
        return get_default_config();
    }
    SchemaDocument schema_doc(origin_schema_doc);
    SchemaValidator validator(schema_doc);

    if (!doc.Accept(validator)) {
        fmt::print(fg(fmt::color::red) | fmt::emphasis::italic,
                   "配置文件无效。将尝试使用默认配置\n");
        fclose(fp);
        return get_default_config();
    }

    // Accept!
    ass_config_t config = {
        .video_width_ = doc["video_width"].GetInt(),
        .video_height_ = doc["video_height"].GetInt(),
        .font_family_ = doc["font_family"].GetString(),
        .font_color_ = 0xFFFFFF, // white
        .font_size_ = 1,
        .font_scale_ = doc["font_scale"].GetFloat(),
        .font_alpha_ = doc["font_alpha"].GetFloat(),
        .font_bold_ = doc["font_bold"].GetBool(),
        .danmaku_show_range_ = doc["danmaku_show_range"].GetFloat(),
        .danmaku_move_time_ = doc["danmaku_move_time"].GetInt(),
        .danmaku_pos_time_ = doc["danmaku_pos_time"].GetInt(),
    };

    fclose(fp);
    return config;
}

} // namespace config