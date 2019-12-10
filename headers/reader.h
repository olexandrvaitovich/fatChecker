
#ifndef FATCHECKER_READER_H
#define FATCHECKER_READER_H

#include <vector>
#include <string>
std::vector<unsigned char> readPart(std::string &filename, int begin, int end);
std::vector<unsigned char> convertToHex(std::string &bytes);
#endif //FATCHECKER_READER_H
