#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "ass_util.hpp"

using namespace std;

void read_file(vector<string> &buffer, string file_name) {
    ifstream in(file_name);

    string line;
    while (std::getline(in, line)) {
        // Line contains string of length > 0 then save it in vector
        if (line.size() > 0)
            buffer.push_back(line);
    }
}

int main() {
    std::string user_name = "live_render";
    std::string tmp_user_name, tmp_content;
    std::string res;
    std::vector<std::string> sc_list;
    int price = 50;
    int font_size = 35;
    int corner_radius = 17;
    int width = 450;
    int x1 = 200;
    int x2 = 200;
    int y1 = 700;
    int y2 = 700;
    int start_time = 1000;

    read_file(sc_list, "sc_content_list.txt");
    for (auto &content : sc_list) {
        tmp_user_name = user_name;
        tmp_content = content;
        sc::sc_item_t sc{tmp_user_name, tmp_content, 0, price};
        ass::SuperChatMessage sc_msg{sc, 0, 0, width, corner_radius, font_size};

        res = sc_msg.getSuperChatAss(x1, y1, x2, y2, start_time, start_time + 1500);

        cout << res;
        start_time += 1500;
        price += 50;
    }

    return 0;
}