#ifndef XML2ASS_FILE_HELPER_H
#define XML2ASS_FILE_HELPER_H


#include <filesystem>
#include <vector>
#include <string>


namespace file_helper {

std::vector<std::string> get_xml_file_list(std::vector<std::string> origin_list);

};


#endif //XML2ASS_FILE_HELPER_H
