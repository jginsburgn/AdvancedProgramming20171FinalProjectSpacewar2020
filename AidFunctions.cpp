// Split function taken from StackOverflow
// http://stackoverflow.com/questions/236129/split-a-string-in-c
#include <sstream>

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}