#include <vector>
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>


#include "reader.h"
#include "parsers.h"

#define PAGE 512

void copyClusters(std::vector<file> &allFiles, std::string &filename, int root_folder_loc, int root_size, int fat_start, int fat_size){
    std::vector<unsigned char> fat = readPart(filename, fat_start , fat_start + fat_size);
    std::vector<int> used_clusters;
    int n = 0;
    for(auto &file:allFiles){
        for(auto i=0;i<file.fat.size();i++){
            if(std::find(used_clusters.begin(), used_clusters.end(), file.fat[i]) == used_clusters.end()){
                used_clusters.emplace_back(file.fat[i]);
            }
            else{
                int cluster_num = findFreeCluster(std::ref(fat));
                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(file.fat[i]-2) , root_folder_loc + root_size+(4*PAGE)*(file.fat[i]-1));
                writeToFile(cluster, filename, root_folder_loc+root_size+(4*PAGE)*(cluster_num-2));
                if(i>0){
                    std::vector<unsigned char> vec = intToUnsignedChar(cluster_num);
                    writeToFile(std::ref(vec) , filename, fat_start+2*file.fat[i-1]);
                    fat = readPart(filename, fat_start , fat_start + fat_size);
                }
                else{
                    std::vector<unsigned char> vec = intToUnsignedChar(cluster_num);
                    writeToFile(std::ref(vec) , filename, root_folder_loc+32*n+26);
                    n++;
                }

            }
        }

    }

}

bool checkCluster(std::vector<unsigned char> cluster){
    for(auto i=0;i<cluster.size();i+=2){
        std::vector<unsigned char> hexabytes = std::vector<unsigned char>(cluster.begin()+i, cluster.begin()+i+2);
        int num = hexbytesToInt(hexabytes);
        if(num==0xfff7) return false;
        }

    return true;
}


std::vector<file> collectEntries(std::vector<file> &allFiles, std::vector<unsigned char> &fat, std::vector<unsigned char> &fat2, std::string &filename, int root_folder_loc, int root_size){
    std::vector<file> toReturn = allFiles;
    for(auto &entry:allFiles){
//        std::cout<<entry.name<<std::endl;

        getFatChain(std::ref(fat), std::ref(fat2), std::ref(entry.fat));
        if(entry.attr["NDIR"]){
            for(auto &k:entry.fat){

                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(k-2) , root_folder_loc + root_size+(4*PAGE)*(k-1));

                if(checkCluster(cluster)){

                    std::vector<file> entryFiles = getFilesFromRootDirectory(cluster);

                    std::vector<file> processedEntryFiles = collectEntries(std::ref(entryFiles), fat, fat2, filename, root_folder_loc, root_size);

                    toReturn.insert( toReturn.end(), processedEntryFiles.begin(), processedEntryFiles.end() );
                }
            }
        }
    }
    return toReturn ;
}

void storeData(std::vector<file> &allFiles, std::vector<unsigned char> &fat, std::vector<unsigned char> &fat2, const int root_folder_loc, const int root_size, std::string &filename, std::string current_dir, std::vector<std::vector<int>> &chains, bool toWrite, int fat_start){

    if(!boost::filesystem::exists(boost::filesystem::path(current_dir))){
        boost::filesystem::create_directory(boost::filesystem::path(current_dir));
    }

    for(auto &entry:allFiles){

        getFatChain(std::ref(fat), std::ref(fat2), std::ref(entry.fat));
    }
    int mm = -1;
    for(auto &entry:allFiles){
        mm++;
        if(!toWrite && mm==allFiles.size()-2){
            break;
        }


//        copyClusters(allFiles, fat, filename, root_folder_loc, root_size, fat_start);


        if(!entry.attr["NDIR"] && static_cast<int>(entry.size/2048+1)!=entry.fat.size()) entry.size = entry.fat.size()*2048;


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

        if(!entry.attr["NDIR"]){

            if(toWrite){
                std::ofstream outfile;
                outfile.open(current_dir+"/"+entry.name+"."+entry.ext, std::ios_base::app);

                for(auto &k:entry.fat) {

                    std::vector<unsigned char> cluster = readPart(filename,
                                                                  root_folder_loc + root_size + (4 * PAGE) * (k - 2),
                                                                  root_folder_loc + root_size + (4 * PAGE) * (k - 1));
                    if(checkCluster(cluster)){
                        for (const auto &c : hexToString(cluster)) outfile << c;
                    }
                }
            }
        }
        else{
            for(auto &k:entry.fat){

                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(k-2) , root_folder_loc + root_size+(4*PAGE)*(k-1));

                if(checkCluster(cluster)){

                    std::vector<file> entryFiles = getFilesFromRootDirectory(cluster);

                    storeData(std::ref(entryFiles), fat, fat2, root_folder_loc, root_size, filename, current_dir+"/"+entry.name, chains, toWrite, fat_start);
                }
            }
        }
    }
}

int main() {

    mbr_info mbrInfo{};
    std::string filename{"../data/hd.img"};

    std::vector<unsigned char> mbr = readPart(filename, 0, PAGE); //reading mbr
//    std::vector<unsigned char> bigPartOfFile = readPart(filename, 0, PAGE * PAGE * 2);

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
        std::vector<unsigned char> fat2 = readPart(filename, fat_start+fat_size , fat_start + fat_size*2);

        for(auto &entry:allFiles){
            getFatChain(std::ref(fat), std::ref(fat2), std::ref(entry.fat));
        }

        auto chains = getAllChains(std::ref(fat), std::ref(fat2));

        storeData(std::ref(allFiles), std::ref(fat), std::ref(fat2), root_folder_loc, root_size, std::ref(filename),"root", std::ref(chains), false, fat_start);

        if(!chains.empty()){
            for(auto j=1;j<chains.size();j++){
                file f;
                f.name = "file"+std::to_string(j);
                f.ext = "txt";
                f.fat.emplace_back(chains[j][0]);
//                getFatChain(std::ref(fat), std::ref(fat2), std::ref(f.fat));
                struct tm date;
                date.tm_yday = 28;
                date.tm_min = 38;
                date.tm_sec = 7;
                f.create_date = date;
                f.attr["NDIR"] = isDir(std::ref(f.fat), std::ref(filename), root_folder_loc, root_size);
                std::vector<unsigned char> entry = parseRootEntryToBytes(f);
                std::vector<unsigned char> hexxxx(entry.begin()+26, entry.begin()+28);
                std::reverse(hexxxx.begin(), hexxxx.end());
                rootDirectory = readPart(filename, root_folder_loc, root_folder_loc + root_size);
                int start = findEmptyByteRoot(rootDirectory);
                writeToFile(entry, filename, start+root_folder_loc);
            }
        }
        rootDirectory = readPart(filename, root_folder_loc, root_folder_loc + root_size);
        fat = readPart(filename, fat_start , fat_start + fat_size);
        fat2 = readPart(filename, fat_start+fat_size , fat_start + fat_size*2);

        allFiles = getFilesFromRootDirectory(rootDirectory);

        allFiles = collectEntries(std::ref(allFiles), std::ref(fat), std::ref(fat2), std::ref(filename), root_folder_loc, root_size);

        for(auto &sss:allFiles) getFatChain(std::ref(fat), std::ref(fat2), std::ref(sss.fat));

        copyClusters(std::ref(allFiles), std::ref(filename), root_folder_loc, root_size, fat_start, fat_size);

        rootDirectory = readPart(filename, root_folder_loc, root_folder_loc + root_size);
        fat = readPart(filename, fat_start , fat_start + fat_size);
        fat2 = readPart(filename, fat_start+fat_size , fat_start + fat_size*2);

        allFiles = getFilesFromRootDirectory(rootDirectory);
        allFiles = collectEntries(std::ref(allFiles), std::ref(fat), std::ref(fat2), std::ref(filename), root_folder_loc, root_size);

        for(auto &sss:allFiles) getFatChain(std::ref(fat), std::ref(fat2), std::ref(sss.fat));


//        storeData(std::ref(allFiles), std::ref(fat), std::ref(fat2), root_folder_loc, root_size, std::ref(filename),"root", std::ref(chains), true, fat_start);

        std::vector<int> used_clusters;
        for(auto &ff:allFiles){
            for(auto hh=0;hh<ff.fat.size();hh++){
                if(std::find(used_clusters.begin(), used_clusters.end(), ff.fat[i]) == used_clusters.end()){
                    used_clusters.emplace_back(ff.fat[i]);
                }
                else{
                    std::cout<< ff.name << " " << ff.fat[i]<<std::endl;
                }
            }
        }
    }
}


