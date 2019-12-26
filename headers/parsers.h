//
// Created by arelav on 29.11.19.
//

#ifndef FATCHECKER_PARSERS_H
#define FATCHECKER_PARSERS_H

#include <vector>
#include <string>
#include <map>

typedef struct mbr_info{
    std::vector<unsigned long> sectors_count; // count of sectors in n-th boot partition
    std::vector<unsigned long> starting_sector; // start of n-th boot sector
    bool fat_flag;
} mbr;

typedef struct boot_info{
    int bytes_per_sector;
    int sectors_per_cluster;
    int number_of_reserved_sectors;
    int num_fat_copies;
    unsigned long total_sectors;
    unsigned long number_root_entries;
    int sectors_per_fat;
} boot;

typedef struct fileDir {
    std::string name;
    std::string ext;
    std::map<std::string, bool> attr;
    std::vector<int> fat;
    tm create_time;
    tm create_date;
    int size;
} file;

#define PAGE 512
tm parseTime(std::vector<unsigned char> &bytes);
tm parseDate(std::vector<unsigned char> &bytes);
void printVector(std::vector<unsigned char> &v);
void fileToBytes(file fileStruct);
int findEmptyByteRoot(std::vector<unsigned char> rootDirectory);
bool IsSubset(std::vector<int> A, std::vector<int> B);
std::vector<unsigned char> reverseBites(std::vector<unsigned char> vect);
auto hexbytesToInt(const std::vector<unsigned char> &b1) -> int;
std::string to_hex_string( const unsigned char i );
bool isDir(std::vector<int> chain, std::string &filename, int root_folder_loc, int root_size);
void getFatChain(std::vector<unsigned char> &fat, std::vector<int> &chain);
std::vector<std::vector<int>> getAllChains(std::vector<unsigned char> &fat);
std::vector<file> getFilesFromRootDirectory(const std::vector<unsigned char> &rootDirectory);
std::string hexToString(std::vector<unsigned char> &vect);
std::map<std::string, bool> parseAttribute(unsigned char val);
void parseMBR(const std::vector<unsigned char> &mbr, mbr_info &mbrInfo);
void parsePartitionEntry(const std::vector<unsigned char> &mbr_part, mbr_info &mbrInfo);
void parseBootSector(const std::vector<unsigned char> &boot, boot_info &bootInfo);
void stringToUnsignedChar(std::string str, std::vector<unsigned char> &arr);
std::string createString(char val, int numbers);
std::vector<unsigned char> parseRootEntryToBytes(file entry);
int findEmptyByteRoot(std::vector<unsigned char> rootDirectory);
std::vector<unsigned char> intToUnsignedChar(int val);
#endif //FATCHECKER_PARSERS_H
