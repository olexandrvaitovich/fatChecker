//
// Created by ovaitovych on 1/16/20.
//

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include "Configs.h"
#ifndef FATCHECKER_CONVERTER_H
#define FATCHECKER_CONVERTER_H

#endif //FATCHECKER_CONVERTER_H

class Converter{
public:
    Converter(Configs config){
        this->config=config;
    }
    auto hexbytesToInt(const std::vector<unsigned char> &b1) -> int{
        int val;
        for (size_t i=0; i < b1.size(); ++i){
            if (i ==0){
                val = (b1[0] << 8*(b1.size()-1));
            } else {
                val |= (b1[i] << (8 * (b1.size()-1) - 8 * i));
            }
        }
        return (int)val;
    }
    std::string hexToString(std::vector<unsigned char> &vect) {
        std::string value = "";
        for (unsigned char j : vect) {
            if (j != ' ') {
                value += to_hex_string(j);
            }
        }
        return value;
    }
    std::vector<unsigned char> intToUnsignedChar(int val) {
        std::stringstream stream;
        stream << std::hex << val;
        std::string result (stream.str());
        int totalLength = std::to_string(val).size();
        int fatResSize = result.size();
        for (int i = 0; i < 4 - fatResSize; i++) {
            result = "0" + result;
        }

        result.insert(2, 1, ' ');

        std::for_each(result.begin(), result.end(), [](char & c){
            c = ::toupper(c);
        });

        std::istringstream hex_chars_stream(result);
        std::vector<unsigned char> bytes;

        unsigned int c;
        while (hex_chars_stream >> std::hex >> c) {
            bytes.push_back(c);
        }
        unsigned char temp = bytes[1];
        bytes[1] = bytes[0];
        bytes[0] = temp;

        return bytes;
    }

    void stringToUnsignedChar(std::string str, std::vector<unsigned char> &arr) {
        for (auto t: str) {
            int val = (int) t;
            arr.emplace_back((unsigned char) val);
        }
    }

    std::string to_hex_string( const unsigned char i ) {
        std::stringstream s;
        s << std::hex << i;
        return s.str();
    }

private:
    Configs config;
};