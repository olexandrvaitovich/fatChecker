//
// Created by ovaitovych on 1/16/20.
//

#ifndef FATCHECKER_DIRPARSER_H
#define FATCHECKER_DIRPARSER_H


#include <vector>
#
class DirParser {
public:
    DirParser(Configs config, ){

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

            std::string name = hexToString(filename);
            std::string extension = hexToString(ext);

            rootFile.name = name;
            rootFile.ext = extension;
            rootFile.attr = parseAttribute(attr[0]);
            std::reverse(creationTime.begin(), creationTime.end());
            rootFile.create_time = parseTime(creationTime);
            std::reverse(creationDate.begin(),creationDate.end());
            rootFile.create_date = parseDate(creationDate);
            std::reverse(fatRef.begin(), fatRef.end());
            rootFile.fat.emplace_back(hexbytesToInt(fatRef));
            std::vector<unsigned char> filesize = std::vector<unsigned char>(rootDirectory.begin() + iter + 28, rootDirectory.begin() + iter + 32);
            std::reverse(filesize.begin(), filesize.end());
            rootFile.size = hexbytesToInt(filesize);

            allFiles.push_back(rootFile);
        }
        return allFiles;
    }
};


#endif //FATCHECKER_DIRPARSER_H
