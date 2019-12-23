#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>


#include "reader.h"
#include "parsers.h"

#define PAGE 512



void storeData(std::vector<file> &allFiles, std::vector<unsigned char> &fat, const int root_folder_loc, const int root_size, std::string &filename, std::string current_dir, std::vector<std::vector<int>> &chains){

    if(!boost::filesystem::exists(boost::filesystem::path(current_dir))){
        boost::filesystem::create_directory(boost::filesystem::path(current_dir));
    }
    for(auto &entry:allFiles){
        getFatChain(fat, std::ref(entry.fat));

        for(auto i=0;i<chains.size();i++){

            auto copy_entry_fat = entry.fat;
            auto copy_chain = chains[i];

            std::sort(copy_entry_fat.begin(), copy_entry_fat.end());
            std::sort(copy_chain.begin(), copy_chain.end());

            if(copy_entry_fat==copy_chain){
                chains.erase(chains.begin()+i);
                break;
            }
        }

        if(entry.attr["NDIR"]){

            std::ofstream outfile;
            outfile.open(current_dir+"/"+entry.name+"."+entry.ext, std::ios_base::app);

            for(auto &k:entry.fat){

                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(k-2) , root_folder_loc + root_size+(4*PAGE)*(k-1));
                for(const auto &c : hexToString(cluster)) outfile << c;
            }
        }
        else{
            for(auto &k:entry.fat){

                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(k-2) , root_folder_loc + root_size+(4*PAGE)*(k-1));

                std::vector<file> entryFiles = getFilesFromRootDirectory(cluster);

                storeData(std::ref(entryFiles), fat, root_folder_loc, root_size, filename, current_dir+"/"+entry.name, chains);
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
        auto chains = getAllChains(std::ref(fat));

        storeData(std::ref(allFiles), std::ref(fat), root_folder_loc, root_size, std::ref(filename),"root", std::ref(chains));

        if(!chains.empty()){
            std::vector<file> lost_clusters;
            for(auto j=0;j<chains.size();j++){
                file f;
                f.name = "file"+std::to_string(j);
                f.ext = "txt";
                f.attr["NDIR"] = true;
                f.fat.emplace_back(chains[j][0]);
                lost_clusters.emplace_back(f);
            }
            storeData(std::ref(lost_clusters), std::ref(fat), root_folder_loc, root_size, std::ref(filename), "root/lost_clusters", std::ref(chains));
            assert(chains.empty());
        }
    }
}
