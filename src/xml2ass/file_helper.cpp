#include "file_helper.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace file_helper {

std::vector<std::string> get_xml_file_list(std::vector<std::string> origin_list) {
    using namespace std::filesystem;

    std::vector<std::string> ret;

    for (auto &item : origin_list) {
        path file_path(item);

        if (!exists(file_path)) {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "{}:无效的路径\n",
                       item);
            continue;
        }

        directory_entry entry(item);
        if (entry.status().type() == file_type::directory) {
            // find all ".xml" files
            directory_iterator dir_file_list(item);

            for (auto &it : dir_file_list) {
                const std::string item_file_name = std::string(it.path().string());
                if (item_file_name.size() > 4) { // ".xml"
                    // case insensitive
                    auto part = item_file_name.substr(item_file_name.size() - 4);
                    // to lower case
                    std::transform(part.begin(), part.end(), part.begin(),
                                   [](unsigned char c) { return std::tolower(c); });

                    if (part == ".xml") {
                        ret.emplace_back(item_file_name);
                    }
                }
            }
        } else {
            // not only handle with normal type
            // respect the user. Do not judge the type of file provided
            ret.emplace_back(item);
        }
    }

    return ret;
}
}; // namespace file_helper