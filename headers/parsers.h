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
    std::string create_time;
} file;

#define PAGE 512

void printVector(std::vector<unsigned char> &v);
std::vector<unsigned char> reverseBites(std::vector<unsigned char> vect);
auto hexbytesToInt(const std::vector<unsigned char> &b1) -> int;
std::string to_hex_string( const unsigned char i );
std::vector<file> getFilesFromRootDirectory(const std::vector<unsigned char> &rootDirectory);
std::string hexToString(std::vector<unsigned char> &vect);
std::map<std::string, bool> parseAttribute(unsigned char val);
void parseMBR(const std::vector<unsigned char> &mbr, mbr_info &mbrInfo);
void parsePartitionEntry(const std::vector<unsigned char> &mbr_part, mbr_info &mbrInfo);
void parseBootSector(const std::vector<unsigned char> &boot, boot_info &bootInfo);
#endif //FATCHECKER_PARSERS_H
