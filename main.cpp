#include <vector>
#include <iostream>
#include <algorithm>


#include "reader.h"
#include "parsers.h"

#define PAGE 512


int main() {
    mbr_info mbrInfo{};
    std::string filename{"../data/hd.img"};

    std::vector<unsigned char> mbr = readPart(filename, 0, PAGE); //reading mbr

    parseMBR(mbr, mbrInfo);

    if (!mbrInfo.fat_flag){
        std::cerr << "There are some regions that are not FAT\n";
        exit(1);
    }

    constexpr int BOOT_LEN = 11;
    boot_info bootInfo{};
    for (unsigned long i : mbrInfo.starting_sector){
        std::vector<unsigned char> boot = readPart(filename, i , i + PAGE);
        parseBootSector(boot, bootInfo);

//        printVector(boot);
//        std::vector<unsigned char> boot1 = readPart(filename, 9216, 9216 + PAGE);

        const int root_folder_loc = bootInfo.num_fat_copies * bootInfo.sectors_per_fat * PAGE + (i + PAGE);
        const int root_size = (bootInfo.number_root_entries * 32 / bootInfo.bytes_per_sector) * PAGE;
        std::vector<unsigned char> rootDirectory = readPart(filename, root_folder_loc, root_folder_loc + root_size);
        std::vector<file> allFiles = getFilesFromRootDirectory(rootDirectory);

        const int fat_start = i + PAGE, fat_size = bootInfo.sectors_per_fat * bootInfo.bytes_per_sector;
        std::vector<unsigned char> fat = readPart(filename, fat_start , fat_start + fat_size);

        file first = allFiles[4];
        std::cout<<i + PAGE*bootInfo.sectors_per_fat*2+PAGE*32+PAGE*4*40;
        std::cout<<first.name;
        //for one file
        for(;;){
            std::vector<unsigned char> num{fat[first.fat.back()*2+1], fat[first.fat.back()*2]};
            auto hexabyte = hexbytesToInt(num);
            if(hexabyte!=65535){
                first.fat.emplace_back(hexabyte);
            }
            else{
                break;
            }
        }
////        for(auto &g:first.fat) std::cout<<g<<std::endl;
        for(auto &k:first.fat){
            std::vector<unsigned char> cluster = readPart(filename, i + PAGE*bootInfo.sectors_per_fat*2+PAGE*32+PAGE*4*k , i + PAGE*bootInfo.sectors_per_fat*2+PAGE*32+PAGE*4*(k+1));
//            std::vector<unsigned char> reversed_cluster = reverseBites(cluster);
            unsigned char buf[2049];
//            std::copy(reversed_cluster.begin(), reversed_cluster.end(), buf);
            std::cout<<hexToString(cluster)<<std::endl;
        }

//        for(auto &i: first.fat) std::cout<<i<<std::endl;
//        std::cout << root_folder_loc + PAGE * 32 + 40 * PAGE << std::endl;
    }
//    std::cout << bootInfo.sectors_per_cluster << " " << bootInfo.sectors_per_fat;
//    std::cout << bootInfo.total_sectors;
}

// second fat addr = hex(34 * 512) + 0x2400 = 0х6800

//root folder addr = hex(2*34*512) + 0x2400 = 0хAC00
