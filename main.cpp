#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>


#include "reader.h"
#include "parsers.h"

#define PAGE 512


void printData(std::vector<file> &allFiles, std::vector<unsigned char> &fat, const int root_folder_loc, const int root_size, std::string &filename, std::string current_dir){
    std::cout<<current_dir<<std::endl;
    if(!boost::filesystem::exists(boost::filesystem::path(current_dir))){
        boost::filesystem::create_directory(boost::filesystem::path(current_dir));
    }
    for(auto &entry:allFiles){
        std::cout<<current_dir+"/"+entry.name+"."+entry.ext<<std::endl;
        for(;;){
            std::vector<unsigned char> num{fat[entry.fat.back()*2+1], fat[entry.fat.back()*2]};
            auto hexabyte = hexbytesToInt(num);
            if(hexabyte!=65535 && hexabyte!=65528){
                entry.fat.emplace_back(hexabyte);
            }
            else{
                break;
            }
        }
        if(entry.name[0]=='P'){
            int n = 111;
        }
        if(entry.attr["NDIR"]){
            std::ofstream outfile;
            outfile.open(current_dir+"/"+entry.name+"."+entry.ext, std::ios_base::app);
            for(auto &k:entry.fat){
                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(k-2) , root_folder_loc + root_size+(4*PAGE)*(k-1));
                for (const auto &c : hexToString(cluster)) outfile << c;
            }
        }
        else{
            for(auto &k:entry.fat){
                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(k-2) , root_folder_loc + root_size+(4*PAGE)*(k-1));
                std::vector<file> entryFiles = getFilesFromRootDirectory(cluster);
                printData(std::ref(entryFiles), fat, root_folder_loc, root_size, filename, current_dir+"/"+entry.name);
            }
        }
    }
}

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

        const int root_folder_loc = bootInfo.num_fat_copies * bootInfo.sectors_per_fat * PAGE + (i + PAGE);

        const int root_size = (bootInfo.number_root_entries * 32 / bootInfo.bytes_per_sector) * PAGE;

        std::vector<unsigned char> rootDirectory = readPart(filename, root_folder_loc, root_folder_loc + root_size);

        std::vector<file> allFiles = getFilesFromRootDirectory(rootDirectory);

        const int fat_start = i + PAGE;
        const int fat_size = bootInfo.sectors_per_fat * bootInfo.bytes_per_sector;

        std::vector<unsigned char> fat = readPart(filename, fat_start , fat_start + fat_size);
        int n = 38;
//        std::cout<<root_folder_loc + root_size+(4*PAGE)*(n-2)<<std::endl;
        file first = allFiles[3];
//        std::cout<<first.name<<std::endl;
//        for(auto &r:allFiles){
//            std::cout<<r.name<<" "<<r.ext<<" "<<std::endl;
//            std::cout << "NDIR: " << r.attr["NDIR"] << std::endl;
//            for(auto &ddd:r.fat) std::cout<<ddd<<" ";
//            std::cout<<std::endl;
//        }
        printData(std::ref(allFiles), std::ref(fat), root_folder_loc, root_size, std::ref(filename),"root");
    }
}
