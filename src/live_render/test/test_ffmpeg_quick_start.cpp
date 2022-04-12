//ffmpeg -i INPUT.mp4 -c copy -movflags faststart STREAMABLE_OUTPUT.mp4
//
//

#include <filesystem>
#include <iostream>

int main() {
    std::filesystem::path cwd = std::filesystem::current_path() / "filename.txt";
    std::cout << "Current path is " << cwd << '\n'; // (1)
//    printf("%s", cwd.c_str());
    return 0;
}
