#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "thirdparty/re2/re2/re2.h"


int main() {

    std::vector<const char *> name_list = {
        // valid
                                            "测试 123",
                                           "COM!",
                                           "Приветмир",
                                           "~~~~~~!",
                                           "123?",
                                           "1COM",
                                           "COM99991",
                                           "COM99991.COM9",
        // invalid
                                           "COM9.COM9",
                                           "COM9.",
                                           "COM9",
                                           "COM9.123"
                                           "123?",
                                           "1<2", 
                                           "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345"};

    RE2 re_device_name(R"(^(COM[0-9]|CON|LPT[0-9]|NUL|PRN|AUX|com[0-9]|con|lpt[0-9]|nul|prn|aux|[\s\.])((\..*)$|$))");
    assert(re_device_name.ok());

    RE2 re_char(R"(\A[^\\\/:*"?<>|]{1,254}\z)");
    assert(re_char.ok());

    std::vector<const char *> valid_list;
    for (auto item : name_list) {
        if (RE2::PartialMatch(item, re_device_name)) {
            printf("invalid item : \"%s\"\n", item);
        }

        valid_list.push_back(item);
    }

    for (auto item : valid_list) {
        if (RE2::PartialMatch(item, re_char) == false) {
            printf("invalid item : \"%s\"\n", item);
        }
    }

    return 0;
}