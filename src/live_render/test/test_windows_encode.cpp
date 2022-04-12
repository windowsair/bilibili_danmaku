#include <algorithm>
#include <cassert>
#include <map>
#include <string>
#include <vector>

#include "thirdparty/simdutf/simdutf.h"

#include "thirdparty/fmt/include/fmt/color.h"
#include "thirdparty/fmt/include/fmt/core.h"

#include "windows.h"

int main() {

    auto fp = fopen("encode_test.txt", "rb");
    if (fp == NULL) {
        perror("Error opening file");
        return (-1);
    }

    std::string file_line(1024, 0);
    while (1) {
        memset(file_line.data(), 0, 1024);
        if (!fgets(file_line.data(), 1023, fp) || ferror(fp) || feof(fp)) {
            break;
        }

        std::string output_path = fmt::format("{}/{}.mp4", "K:/ff/", file_line);

        std::wstring utf16_cov;
        utf16_cov.resize((output_path.size() + 1) * 6);

        std::string local_code_page_str;
        local_code_page_str.resize((output_path.size() + 1) * 6);

        // convert to UTF-16LE
        size_t utf16words = simdutf::convert_utf8_to_utf16(
            output_path.data(), output_path.size(), (char16_t *)utf16_cov.data());

        auto ret = WideCharToMultiByte(CP_OEMCP, NULL, utf16_cov.data(), -1,
                                       local_code_page_str.data(),
                                       local_code_page_str.size(), NULL, NULL);

        assert(ret != 0);

        printf("%s\n", local_code_page_str.c_str());
    }
    return 0;
}