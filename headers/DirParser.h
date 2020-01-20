//
// Created by ovaitovych on 1/16/20.
//

#ifndef FATCHECKER_DIRPARSER_H
#define FATCHECKER_DIRPARSER_H


#include <vector>
#include <algorithm>
#include "Configs.h"
#include "Converter.h"
class DirParser {
public:
    DirParser(Configs config){
        this->config = config;
    }
    std::vector<file> getFilesFromRootDirectory(const std::vector<unsigned char> &rootDirectory) {
        std::vector<file> allFiles;
        for (int iter = 0;;iter+=32) {
            if(iter==rootDirectory.size()){
                break;
            }
            std::vector<unsigned char> filename = std::vector<unsigned char>(rootDirectory.begin() + iter, rootDirectory.begin() + iter + 8);
            if ((!isalpha(filename[0]) && !isdigit(filename[0])) || filename[0] == 0) {
                continue;
            }

            file rootFile;

            std::vector<unsigned char> ext = std::vector<unsigned char>(rootDirectory.begin() + iter + 8,
                                                                        rootDirectory.begin() + iter + 11);
            std::vector<unsigned char> attr = std::vector<unsigned char>(rootDirectory.begin() + iter + 11, rootDirectory.begin() + iter + 12);
            std::vector<unsigned char> creationTime = std::vector<unsigned char>(rootDirectory.begin() + iter + 22, rootDirectory.begin() + iter + 24);
            std::vector<unsigned char> creationDate = std::vector<unsigned char>(rootDirectory.begin() + iter + 24, rootDirectory.begin() + iter + 26);
            std::vector<unsigned char> fatRef = std::vector<unsigned char>(rootDirectory.begin() + iter + 26, rootDirectory.begin() + iter + 28);

            std::string name = converter.hexToString(filename);
            std::string extension = converter.hexToString(ext);

            rootFile.name = name;
            rootFile.ext = extension;
            rootFile.attr = parseAttribute(attr[0]);
            std::reverse(creationTime.begin(), creationTime.end());
            rootFile.create_time = parseTime(creationTime);
            std::reverse(creationDate.begin(),creationDate.end());
            rootFile.create_date = parseDate(creationDate);
            std::reverse(fatRef.begin(), fatRef.end());
            rootFile.fat_chain.emplace_back(converter.hexbytesToInt(fatRef));
            std::vector<unsigned char> filesize = std::vector<unsigned char>(rootDirectory.begin() + iter + 28, rootDirectory.begin() + iter + 32);
            std::reverse(filesize.begin(), filesize.end());
            rootFile.size = converter.hexbytesToInt(filesize);

            allFiles.push_back(rootFile);
        }
        return allFiles;
    }

private:
    Configs config;
    Converter converter = Converter(config);

    std::map<std::string, bool> parseAttribute(unsigned char val) {
        std::map<int, std::string> values = {{0, "RO"},
                                             {1, "HID"},
                                             {2, "SYSF"},
                                             {3, "VOLL"},
                                             {4, "NDIR"},
                                             {5, "ARCH"}};
        std::map<std::string, bool> attrbiutes;
        for (int i = 0; i < 6; i++) {

            int decVal = ((int) val >> i & 1);
            attrbiutes[values[i]] = decVal;
        }
        return attrbiutes;
    }

    tm parseTime(std::vector<unsigned char> &bytes){
        struct tm time;
        time.tm_hour = (int)(bytes[0] >> 3);
        time.tm_min = (int)((bytes[0] & 0b111) << 3 | bytes[1] >> 5);
        time.tm_sec = (int)(bytes[1] & 0b111) * 2;

        return time;
    }

    tm parseDate(std::vector<unsigned char> &bytes){
        struct tm time;
        time.tm_yday = (int)(bytes[0] >> 1) + 1980;
        time.tm_min = (int)((bytes[0] & 0b1) << 5  | bytes[1] >> 4);
        time.tm_sec = (int)(bytes[1] & 0b11111);

        return time;
    }
};


#endif //FATCHECKER_DIRPARSER_H
