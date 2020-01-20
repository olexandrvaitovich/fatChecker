#ifndef FATCHECKER_CONFIGS_H
#define FATCHECKER_CONFIGS_H

#include <vector>
#include <string>
#include <map>

#define PAGE 512

typedef struct mbr_info{
    std::vector<unsigned long> sectors_count; // count of sectors in n-th boot partition
    std::vector<unsigned long> starting_sector; // start of n-th boot sector
} Mbr;

typedef struct boot_info{
    int bytes_per_sector;
    int sectors_per_cluster;
    int number_of_reserved_sectors;
    int num_fat_copies;
    unsigned long total_sectors;
    unsigned long first_cluster_of_root;
    int sectors_per_fat;
} Boot;

class Configs {
public:
    Mbr mbr;
    Boot boot;



};

typedef struct lognFileDir {
    unsigned char sequenceNumber;
    std::string nameCharacters;
} longFile;


typedef struct file_info {
    std::string longName;
    std::string name;
    std::string ext;
    std::map<std::string, bool> attr;
    std::vector<int> fat_chain;
    tm create_time;
    tm create_date;
    int size;
} file;

#endif //FATCHECKER_CONFIGS_H
