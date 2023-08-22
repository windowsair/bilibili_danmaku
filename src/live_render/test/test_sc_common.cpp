#include "test_sc_common.h"
#include "thirdparty/pugixml/pugixml.hpp"

inline int getItemAliveTime(int price) {
    if (price < 50) {
        return 60 * 1000;
    } else if (price < 100) {
        return 2 * 60 * 1000;
    } else if (price < 500) {
        return 5 * 60 * 1000;
    } else if (price < 1000) {
        return 30 * 60 * 1000;
    } else if (price < 2000) {
        return 60 * 60 * 1000;
    }

    return 2 * 60 * 60 * 1000;
}

int process_sc_list_from_file(
    std::string &file_path,
    moodycamel::ReaderWriterQueue<std::vector<sc::sc_item_t>> &queue) {
    pugi::xml_document doc;
    pugi::xml_parse_result parse_result;
    std::vector<sc::sc_item_t> sc_list;

    parse_result = doc.load_file(file_path.c_str());

    if (parse_result.status == pugi::status_end_element_mismatch) {
        // maybe <i> mismatch in tail
    }

    auto root_node = doc.child("i");
    if (!root_node) {
        return 0;
    }

    // parse sc list
    std::string user_name, sc_content;
    int price;
    int a, b;
    int time, base_time = 0, max_alive_time = 0;
    for (auto &node : root_node.children("sc")) {
        user_name = node.attribute("user").value();
        price = std::stoi(node.attribute("price").value());
        sc_content = node.text().get();
        // 123.456
        sscanf(node.attribute("ts").value(), "%d.%d", &a, &b);
        time = a * 1000 + b;
        if (base_time == 0) {
            base_time = time - 1000;
        }
        time -= base_time;
        max_alive_time = (std::max)(max_alive_time, time + getItemAliveTime(price));

        // add to queue
        sc_list.emplace_back(user_name, sc_content, time, price);
    }
    queue.enqueue(std::move(sc_list));
    max_alive_time += 1000 * 60; // 1min

    return max_alive_time;
}