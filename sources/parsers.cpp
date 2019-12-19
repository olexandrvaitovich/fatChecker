#include "parsers.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <bitset>

void printVector(std::vector<unsigned char> &v){
    for (auto &m: v){
        std::cout << std::hex << (int)m << " ";
    }
}

std::string to_hex_string( const unsigned char i ) {
    std::stringstream s;
    s << std::hex << i;
    return s.str();
}
//std::string parseTime(std::vector<unsigned char>& time){
//    std::string res;
//    return std::to_string(time[0]) + " " + std::to_string(time[1] ) +
//            " " + std::to_string(time[2]) + " " + std::to_string(time[3]);
////    return (int)
////    return res;
//
//}

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
//        std::vector<unsigned char> creationTime = std::vector<unsigned char>(rootDirectory.begin() + iter + 22, rootDirectory.begin() + iter + 26);
        std::vector<unsigned char> fatRef = std::vector<unsigned char>(rootDirectory.begin() + iter + 26, rootDirectory.begin() + iter + 28);

        std::string name = hexToString(filename);
        std::string extension = hexToString(ext);

        rootFile.name = name;
        rootFile.ext = extension;
        rootFile.attr = parseAttribute(attr[0]);
        std::reverse(fatRef.begin(), fatRef.end());
        rootFile.fat.emplace_back(hexbytesToInt(fatRef));



        allFiles.push_back(rootFile);
    }
    return allFiles;
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

std::vector<unsigned char> reverseBites(std::vector<unsigned char> vect) {
    for (int i = 0; i < vect.size() - 1; i += 2) {
        unsigned char temp = vect[i];
        vect[i] = vect[i + 1];
        vect[i + 1] = temp;
    }
    return vect;
}

std::map<std::string, bool> parseAttribute(unsigned char val) {
    std::map<int, std::string> values = {{0, "RO"},
                                         {1, "HID"},
                                         {2, "SYSF"},
                                         {3, "VOLL"},
                                         {4, "NDIR"},
                                         {5, "ARCH"}};
    std::map<std::string, bool> attrbiutes;
    for (int i = 0; i < 6; i++) {
        int decVal = ((int) val >> (i + 1) & 1);
        attrbiutes[values[i]] = decVal;
    }
    return attrbiutes;
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

void parseBootSector(const std::vector<unsigned char> &boot, boot_info &bootInfo){
    std::vector<int> offsets{11,13,14,16,17,19,22};
    std::vector<int> lengths{2,1,2,1,2,2,2};

    auto temp_vect =  std::vector<unsigned char>
            (boot.begin() + offsets[0],boot.begin() + offsets[0]+ lengths[0]);
    std::reverse(temp_vect.begin(), temp_vect.end());
    bootInfo.bytes_per_sector = hexbytesToInt(temp_vect);
//    std::cout << bootInfo.bytes_per_sector<<"\n";

    bootInfo.sectors_per_cluster = (int)boot[offsets[1]];

    temp_vect = std::vector<unsigned char>
            (boot.begin() + offsets[2],boot.begin() + offsets[2]+ lengths[2]);
    std::reverse(temp_vect.begin(), temp_vect.end());
    bootInfo.number_of_reserved_sectors = hexbytesToInt(temp_vect);

    bootInfo.num_fat_copies = boot[offsets[3]];


    temp_vect = std::vector<unsigned char>
            (boot.begin() + offsets[4],boot.begin() + offsets[4]+ lengths[4]);
    std::reverse(temp_vect.begin(), temp_vect.end());
    bootInfo.number_root_entries = hexbytesToInt(temp_vect);

    temp_vect = std::vector<unsigned char>
            (boot.begin() + offsets[5],boot.begin() + offsets[5]+ lengths[5]);
    std::reverse(temp_vect.begin(), temp_vect.end());
    bootInfo.total_sectors = hexbytesToInt(temp_vect);

    temp_vect = std::vector<unsigned char>
            (boot.begin() + offsets[6],boot.begin() + offsets[6]+ lengths[6]);
    std::reverse(temp_vect.begin(), temp_vect.end());
    bootInfo.sectors_per_fat = hexbytesToInt(temp_vect);
}




void parseMBR(const std::vector<unsigned char> &mbr, mbr_info &mbrInfo){
    int offsets[]{446,462,478,494};
    constexpr int BOOT_SIZE_BYTES = 16; // ya ebu 16 chi 17

    for (auto &offset: offsets){
        parsePartitionEntry(std::vector<unsigned char> (mbr.begin() + offset, mbr.begin() + offset + BOOT_SIZE_BYTES), mbrInfo);
    }
}


void parsePartitionEntry(const std::vector<unsigned char> &mbr_part, mbr_info &mbrInfo){
    if ((int)mbr_part[0] == 0 && (int)mbr_part[1] == 0){ // this boot block is disabled
        return;
    }
    constexpr int filesystem_type_offset = 4, fat16 = 4, fat16B = 6;
    mbrInfo.fat_flag = ((int)mbr_part[filesystem_type_offset] == fat16 || (int)mbr_part[filesystem_type_offset] == fat16B);

    constexpr int address_beg = 8, address_end = 12;
    auto address = std::vector<unsigned char>(mbr_part.begin() + address_beg, mbr_part.begin() + address_end);
    std::reverse(address.begin(), address.end());

    unsigned long hex_val = (address[0] << 24) | (address[1] << 16) |
                            (address[2] << 8) | (address[3]); // make a digit from 4 hex values
    mbrInfo.starting_sector.emplace_back((int)hex_val * PAGE);

    constexpr int size_beg = 12, size_end = 16;
    auto size = std::vector<unsigned char>(mbr_part.begin() + size_beg, mbr_part.begin() + size_end);
    std::reverse(size.begin(), size.end());

    hex_val = (size[0] << 24) | (size[1] << 16) | (size[2] << 8) | (size[3]);
    mbrInfo.sectors_count.emplace_back((int)hex_val * PAGE);
}
