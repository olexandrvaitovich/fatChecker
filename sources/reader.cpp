#include <fstream>
#include <sstream>
#include "reader.h"


std::vector<unsigned char> readPart(std::string &filename, int begin, int end){
    unsigned char x;
    std::ifstream input(filename, std::ios::binary);
    input >> std::noskipws;
    input.seekg(begin);

    std::string results;
    for  (size_t i=0; i < end - begin; ++i) {
        input >> x;
        std::stringstream stream;
        stream << std::hex << static_cast<int>(x);
        results += (stream.str() + " ");
    }
    return convertToHex(results);
}

std::vector<unsigned char> convertToHex(std::string &bytes){
    std::istringstream hex_chars_stream(bytes);
    std::vector<unsigned char> new_bytes;

    unsigned int c;
    while (hex_chars_stream >> std::hex >> c)
    {
        new_bytes.push_back(c);
    }
    return new_bytes;
}

