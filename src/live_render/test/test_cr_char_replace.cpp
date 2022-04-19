#include <cassert>
#include <iostream>
#include <string>

#include "thirdparty/re2/re2/re2.h"

int main() {
    std::string str;

    // The original string we need to match is \\r
    RE2 re(R"(\\\\r)");
    assert(re.ok());

    // test
    str = R"(\\r123\\r)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "123");

    str = R"(\r1\\2r3\r)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == R"(\r1\\2r3\r)");

    str = R"(123\\r)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "123");

    str = R"(\\r)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "");

    str = R"(\\r\\\\r)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == R"(\\)");

    str = R"(\r)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "\\r");

    str = "\r";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "\r");

    str = "\\\r";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "\\\r");

    str = "\\\\\r";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "\\\\\r");

    str = R"(\\r帅\\r帅\\r帅)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == "帅帅帅");

    str = R"(\\\\r帅\\r帅)";
    RE2::GlobalReplace(&str, re, "");
    assert(str == R"(\\帅帅)");

    std::cout << "TEST PASSED" << std::endl;
    return 0;
}