#include "parsers.h"
#include "reader.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <bits/stdc++.h>

void printVector(std::vector<unsigned char> &v){
    for (auto &m: v){
        std::cout << std::hex << (int)m << " ";
    }
}

bool IsSubset(std::vector<int> A, std::vector<int> B)
{
    std::sort(A.begin(), A.end());
    std::sort(B.begin(), B.end());
    return std::includes(A.begin(), A.end(), B.begin(), B.end());
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
//

void stringToUnsignedChar(std::string str, std::vector<unsigned char> &arr) {
    for (auto t: str) {
        int val = (int) t;
        arr.emplace_back((unsigned char) val);
    }
}

std::string createString(char val, int numbers) {
    std::string str;
    for (int i = 0; i < numbers; ++i) {
        str += val;
    }
    return str;
}


tm parseDate(std::vector<unsigned char> &bytes){
    struct tm time;
    time.tm_yday = (int)(bytes[0] >> 1) + 1980;
    time.tm_min = (int)((bytes[0] & 0b1) << 5  | bytes[1] >> 4);
    time.tm_sec = (int)(bytes[1] & 0b11111);

    return time;
}

std::vector<unsigned char> parseRootEntryToBytes(file entry) {
    std::vector<unsigned char> rootEntry;
    stringToUnsignedChar(entry.name, rootEntry); // NAME
    stringToUnsignedChar(createString(' ', 8 - entry.name.size()), rootEntry); // NAME TO THE END
    stringToUnsignedChar(entry.ext, rootEntry); // EXTENSION
    stringToUnsignedChar(createString(' ', 3 - entry.ext.size()), rootEntry); // EXTENSION TO THE END
    std::vector<std::string> attrValues = {"ARCH", "NDIR", "VOLL", "SYSF", "HID", "RO"};
    std::string attributes;
    for(auto val : attrValues) {
        attributes += std::to_string(entry.attr[val]);
    }
    std::reverse(attributes.begin(), attributes.end());
    unsigned char attrByte = (unsigned char) std::stoi(attributes, 0, 2);
    rootEntry.emplace_back(attrByte); // ATTRIBUTE
    for (int i = 0; i < 14; i++) { // FAT
        unsigned val = 0;
        rootEntry.emplace_back(val);
    }

    std::stringstream stream; // FAT
    stream << std::hex << entry.fat[0];
    std::string result (stream.str());
    int fatResSize = result.size();
    for (int i = 0; i < 4 - fatResSize; i++) {
        result = "0" + result;
    }
    std::string fatBytes;
    for (int i = 0; i < 5; i++) {
        if (i == 2) {
            fatBytes += ' ';
        } else {
            fatBytes += result[i];
        }
    }
    std::istringstream hex_chars_stream(fatBytes);
    std::vector<unsigned char> bytes;

    unsigned int c;
    while (hex_chars_stream >> std::hex >> c) {
        bytes.push_back(c);
    }
    rootEntry.emplace_back(bytes[1]);
    rootEntry.emplace_back(bytes[0]);

    for (int i = 0; i < 4; i++) { // LAST EMPTY BYTES
        unsigned val = 0;
        rootEntry.emplace_back(val);
    }

    return rootEntry;
}

int findEmptyByteRoot(std::vector<unsigned char> rootDirectory) {
    for (int iter = 0;;iter+=32) {
        if (iter == rootDirectory.size()) {
            break;
        }
        if ((!isalpha(rootDirectory[iter]) && !isdigit(rootDirectory[iter])) || rootDirectory[iter] == 0) {
            return iter;
        }
    }
    return -1;
}

bool isDir(std::vector<int> chain, std::string &filename, int root_folder_loc, int root_size){
    for(auto k=0;k<chain.size();k++){
        std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(chain[k]-2) , root_folder_loc + root_size+(4*PAGE)*(chain[k]-1));
        for(auto i=0;i<cluster.size();i+=32){
            bool status=true;
            for(auto j=0;j<11;j++){
                if(cluster[i+j]==0 || (!isalpha(cluster[i+j]) && !isdigit(cluster[i+j]) && cluster[i+j]!=' ')){
                    status = false;
                    break;
                }
            }
            if(status){
                if((int)cluster[i+11]>63) status=false;
                if(status){
                    std::vector<unsigned char> fatRef = std::vector<unsigned char>(cluster.begin() + i + 26, cluster.begin() + i + 28);
                    std::reverse(fatRef.begin(), fatRef.end());
                    if(hexbytesToInt(fatRef)>17408) status=false;
                }
            }
            if(status) return true;
        }
    }
    return false;
}


void getFatChain(std::vector<unsigned char> &fat, std::vector<int> &chain){
    for(;;){

        std::vector<unsigned char> num{fat[chain.back()*2+1], fat[chain.back()*2]};
        auto hexabyte = hexbytesToInt(num);

        if(hexabyte==0){
            chain.clear();
            break;
        }
        else if(hexabyte<65528) chain.emplace_back(hexabyte);
        else break;
    }
}

std::vector<std::vector<int>> getAllChains(std::vector<unsigned char> &fat){

    std::vector<std::vector<int>> chains;
    for(auto i=2;i*2+1<fat.size();i++){

        std::vector<int> chain;
        chain.emplace_back(i);
        getFatChain(fat, std::ref(chain));

        if(!chain.empty()){
            chains.emplace_back(chain);
            chain.clear();
        }
    }
    for(auto i=0;i<chains.size();i++){
        for(auto j=0;j<chains.size();j++){

            if(i!=j){
                auto a = chains[i];
                auto b = chains[j];
                if(IsSubset(chains[i], chains[j])){
                    chains[j].clear();
                }
            }
        }
    }
    for(auto i=0;i<chains.size();){
        if(chains[i].empty()) chains.erase(chains.begin()+i);
        else i++;
    }

    return chains;
}

tm parseTime(std::vector<unsigned char> &bytes){
    struct tm time;
    time.tm_hour = (int)(bytes[0] >> 3);
    time.tm_min = (int)((bytes[0] & 0b111) << 3 | bytes[1] >> 5);
    time.tm_sec = (int)(bytes[1] & 0b111) * 2;

    return time;
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


std::string tohex(const std::string& s)
{
    std::ostringstream ret;

    unsigned int c;
    for (std::string::size_type i = 0; i < s.length(); ++i)
    {
        c = (unsigned int)(unsigned char)s[i];
        ret << std::hex << std::setfill('0') << c;
    }
    return ret.str();
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

        int decVal = ((int) val >> i & 1);
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
