#include <fstream>
#include <sstream>
#include "reader.h"
#include <algorithm>


int findFreeCluster(std::vector<file> &allFiles){
    for(auto i=2;i<8704;i++){
        bool status = true;
        for(auto &file:allFiles){
            if(std::find(file.fat.begin(), file.fat.end(), i)!=file.fat.end()){
                status = false;
                break;
            }
        }
        if(status) return i;
    }
    return -1;
}

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

void writeToFile(std::vector<unsigned char> &vect, std::string const &name, size_t pos) {
    std::fstream s(name);
    s.seekp(pos);
    s.write((char *)&vect[0], vect.size());
}

void copyCluster(std::string &filename, size_t from_pos, size_t to_pos, int sectors_per_cluster){
    std::vector<unsigned char> cl1 = readPart(filename, from_pos, from_pos + 512 * sectors_per_cluster);

    writeToFile(cl1, filename, to_pos);
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

