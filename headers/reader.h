
#ifndef FATCHECKER_READER_H
#define FATCHECKER_READER_H

#include <vector>
#include <string>
#include "parsers.h"

std::vector<unsigned char> readPart(std::string &filename, int begin, int end);
std::vector<unsigned char> convertToHex(std::string &bytes);
void writeToFile(std::vector<unsigned char> &vect, std::string const &name, size_t pos);
int findFreeCluster(std::vector<unsigned char> &fat);
#endif //FATCHECKER_READER_H
