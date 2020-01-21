#include <vector>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <Configs.h>


#include "reader.h"
#include "parsers.h"

#define PAGE 512

void copyPart(std::string &filename, size_t from_pos, size_t to_pos, int bytes){
    std::vector<unsigned char> part = readPart(filename, from_pos, from_pos + bytes);

    writeToFile(part, filename, to_pos);
}

std::vector<unsigned char>::iterator clusterIsEmpty(std::vector<unsigned char> &cluster, int count){//how many clusters to skip
    for (auto i=cluster.begin(); i < cluster.end(); i += 32){
        if (std::unique(i, i + 32) == i + 1 && *i == 0){
            count--;
            if (count == 0){
                return i;
            }
        }
    }
    return cluster.end();
}

void checkLinks(std::vector< unsigned char> mine_entry, std::vector<unsigned char> fathers_entry,
                int first_cluster_addr, std::string &filename){

    auto temp = std::vector<unsigned char>(mine_entry.begin() + 26, mine_entry.end() + 28);
    std::reverse(temp.begin(), temp.end());
    auto cluster_addr = hexbytesToInt(temp) * 512 * 4 + first_cluster_addr;
    auto cluster = readPart(filename, cluster_addr, cluster_addr + 512 * 4);

    auto free_cluster1 = clusterIsEmpty(cluster, 1);
    auto free_cluster2 = clusterIsEmpty(cluster, 2);

    if ((free_cluster1 != cluster.end()) && (free_cluster2 != cluster.end())){
        int inx1 = free_cluster1 - cluster.begin(), inx2 = free_cluster2 - cluster.begin();
        copyPart(filename, cluster_addr, inx1, 32);
        copyPart(filename, cluster_addr + 32, inx2, 32);

        mine_entry[0] = 0x2E;
        fathers_entry[0] = 0x2E;
        fathers_entry[1] = 0x2E;

        writeToFile(mine_entry,filename, cluster_addr);
        writeToFile(fathers_entry,filename, cluster_addr);
    }

}

int detectLocation(file entry, int root_folder_loc){
    std::string filename{"../data/hd.img"};
    for(auto i=0;i<17831916;i+=32){
        std::vector<unsigned char> vec = readPart(filename, root_folder_loc + i, root_folder_loc + i + 32);
            std::vector<unsigned char> filename = std::vector<unsigned char>(vec.begin(), vec.begin() + 8);
            std::string name = hexToString(filename);
            if (name == entry.name) {
                return root_folder_loc + i;
            }

    }
    return -1;
}

void copyClusters(std::vector<file> &allFiles, std::string &filename, int root_folder_loc, int root_size, int fat_start, int fat_size){
    std::vector<unsigned char> fat = readPart(filename, fat_start , fat_start + fat_size);
    std::vector<int> used_clusters;
    for(auto &file:allFiles){
        for(auto i=0;i<file.fat_chain.size();i++){
            if(std::find(used_clusters.begin(), used_clusters.end(), file.fat_chain[i]) == used_clusters.end()){
                used_clusters.emplace_back(file.fat_chain[i]);
            }
            else{
                std::vector<unsigned char> fat = readPart(filename, fat_start , fat_start + fat_size);
                int cluster_num = findFreeCluster(std::ref(fat));
                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc + root_size+(4*PAGE)*(file.fat_chain[i]-2) , root_folder_loc + root_size+(4*PAGE)*(file.fat_chain[i]-1));
                writeToFile(cluster, filename, root_folder_loc+root_size+(4*PAGE)*(cluster_num-2));
                if(i>0){
                    std::vector<unsigned char> vec = intToUnsignedChar(cluster_num);
                    std::vector<unsigned char> vec2 = intToUnsignedChar(0xffff);
                    writeToFile(std::ref(vec) , filename, fat_start+2*file.fat_chain[i-1]);
                    file.fat_chain[i] = cluster_num;
                    writeToFile(std::ref(vec2) , filename, fat_start+2*file.fat_chain[i]);
                    used_clusters.emplace_back(file.fat_chain[i]);
                }
                else{
                    std::vector<unsigned char> vec = intToUnsignedChar(cluster_num);
                    std::vector<unsigned char> vec2 = intToUnsignedChar(0xffff);
                    auto fffff = detectLocation(file, root_folder_loc);
                    writeToFile(std::ref(vec) , filename, fffff+26);
                    file.fat_chain[i] = cluster_num;
                    writeToFile(std::ref(vec2) , filename, fat_start+2*file.fat_chain[i]);
                    used_clusters.emplace_back(file.fat_chain[i]);
                }
            }
        }
    }
}

//bool checkCluster(std::vector<unsigned char> cluster){
//    for(auto i=0;i<cluster.size();i+=4){
//        std::vector<unsigned char> hexabytes = std::vector<unsigned char>(cluster.begin()+i, cluster.begin()+i+2);
//        int num = hexbytesToInt(hexabytes);
//        if(num==0xfff7) return false;
//        }
//
//    return true;
//}


std::vector<file> collectEntries(std::vector<file> &allFiles, std::vector<unsigned char> &fat, std::vector<unsigned char> &fat2, std::string &filename, int root_folder_loc, int root_size){
    std::vector<file> toReturn = allFiles;
    for(auto &entry:allFiles){
        getFatChain(std::ref(fat), std::ref(fat2), std::ref(entry.fat_chain));
        if(entry.attr["NDIR"]){
            for(auto &k:entry.fat_chain){

                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc+(8*PAGE)*(k-2) , root_folder_loc+(8*PAGE)*(k-1));

//                if(checkCluster(cluster)){

                    std::vector<file> entryFiles = getFilesFromRootDirectory(cluster);

                    for (auto &son: entryFiles){
                        checkLinks(parseRootEntryToBytes(son), parseRootEntryToBytes(entry),root_folder_loc+root_size ,filename);
                    }

                    std::vector<file> processedEntryFiles = collectEntries(std::ref(entryFiles), fat, fat2, filename, root_folder_loc, root_size);

                    toReturn.insert( toReturn.end(), processedEntryFiles.begin(), processedEntryFiles.end() );
//                }
            }
        }
    }
    return toReturn ;
}

void storeData(std::vector<file> &allFiles, std::vector<unsigned char> &fat, std::vector<unsigned char> &fat2, const int root_folder_loc, const int root_size, std::string &filename, std::string current_dir, std::vector<std::vector<int>> &chains, bool toWrite, int fat_start){

    if(!std::filesystem::exists(std::filesystem::path(current_dir))){
        std::filesystem::create_directory(std::filesystem::path(current_dir));
    }

    for(auto &entry:allFiles){
        getFatChain(std::ref(fat), std::ref(fat2), std::ref(entry.fat_chain));
    }
    int mm = -1;
    for(auto &entry:allFiles){
        std::cout<<current_dir<<"/"<<entry.longName<<std::endl;
        mm++;
        if(!toWrite && mm==allFiles.size()-2){
            break;
        }

        if(!entry.attr["NDIR"] && static_cast<int>(entry.size/4096+1)!=entry.fat_chain.size()) entry.size = entry.fat_chain.size()*4096;


        for(auto i=0;i<chains.size();i++){

            auto copy_entry_fat = entry.fat_chain;
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

                for(auto &k:entry.fat_chain) {

                    std::vector<unsigned char> cluster = readPart(filename,
                                                                  root_folder_loc + (8 * PAGE) * (k - 2),
                                                                  root_folder_loc + (8 * PAGE) * (k - 1));
//                    if(checkCluster(cluster)){
                        for (const auto &c : hexToString(cluster)) outfile << c;
//                    }
                }
            }
        }
        else{
            for(auto &k:entry.fat_chain){

                std::vector<unsigned char> cluster = readPart(filename, root_folder_loc +(8*PAGE)*(k-2) , root_folder_loc +(8*PAGE)*(k-1));

//                if(checkCluster(cluster)){

                    std::vector<file> entryFiles = getFilesFromRootDirectory(cluster);

                    storeData(std::ref(entryFiles), fat, fat2, root_folder_loc, root_size, filename, current_dir+"/"+entry.longName, chains, toWrite, fat_start);
//                }
            }
        }
    }
}

int main() {

    Configs config;
    std::string filename{"../data/fat32fs.img"};

    std::vector<unsigned char> mbr = readPart(filename, 0, PAGE); //reading mbr

    parseMBR(mbr, config.mbr);

    for (unsigned long i : config.mbr.starting_sector){

        std::vector<unsigned char> boot = readPart(filename, i , i + PAGE);

        parseBootSector(boot, config.boot);

        std::vector<int> rootChain;
        rootChain.emplace_back(config.boot.first_cluster_of_root);

        const int fat_start = i + PAGE*config.boot.number_of_reserved_sectors;
        const int fat_size = config.boot.sectors_per_fat * config.boot.bytes_per_sector;

        std::vector<unsigned char> fat = readPart(filename, fat_start , fat_start + fat_size);
        std::vector<unsigned char> fat2 = readPart(filename, fat_start+fat_size , fat_start + fat_size*2);

        getFatChain(std::ref(fat), std::ref(fat2), std::ref(rootChain));

        std::vector<file> allFiles;

        for(auto ind=0;ind<rootChain.size();ind++){
            auto cl_start = fat_start+fat_size*2 + (config.boot.sectors_per_cluster*PAGE)*(rootChain[ind]-2);
            auto cl_end = cl_start+(config.boot.sectors_per_cluster*PAGE)*(rootChain[ind]-1);
            std::vector<unsigned char> rootDirectory = readPart(filename, cl_start, cl_end);
            std::vector<file> tempFiles = getFilesFromRootDirectory(rootDirectory);
            allFiles.insert(allFiles.end(), tempFiles.begin(), tempFiles.end());
        }

        auto chains = getAllChains(std::ref(fat), std::ref(fat2));

        auto root_folder_loc = fat_start+fat_size*2;
        auto root_size = PAGE*config.boot.sectors_per_cluster;

        storeData(std::ref(allFiles), std::ref(fat), std::ref(fat2), root_folder_loc, root_size, std::ref(filename),"root", std::ref(chains), false, fat_start);

        if(!chains.empty()){
            for(auto j=1;j<chains.size();j++){
                file f;
                f.name = "file"+std::to_string(j);
                f.ext = "txt";
                f.fat_chain.emplace_back(chains[j][0]);
                getFatChain(std::ref(fat), std::ref(fat2), std::ref(f.fat_chain));
                struct tm date;
                date.tm_yday = 28;
                date.tm_min = 38;
                date.tm_sec = 7;
                f.create_date = date;
                f.attr["NDIR"] = isDir(std::ref(f.fat_chain), std::ref(filename), root_folder_loc, root_size);
                std::vector<unsigned char> entry = parseRootEntryToBytes(f);
                int start;
                int clst_num;
                for(auto ind=0;ind<rootChain.size();ind++){
                    auto cl_start = fat_start+fat_size*2 + (config.boot.sectors_per_cluster*PAGE)*(rootChain[ind]-2);
                    auto cl_end = cl_start+(config.boot.sectors_per_cluster*PAGE)*(rootChain[ind]-1);
                    std::vector<unsigned char> rootDirectory = readPart(filename, cl_start, cl_end);
                    start = findEmptyByteRoot(rootDirectory, entry.size());
                    if(start!=-1){
                        clst_num = rootChain[ind];
                        break;
                    }

                }
                writeToFile(entry, filename, root_folder_loc+ (config.boot.sectors_per_cluster*PAGE)*(clst_num-2));
            }
        }
        auto rootDirectory = readPart(filename, root_folder_loc, root_folder_loc + root_size);
        fat = readPart(filename, fat_start , fat_start + fat_size);
        fat2 = readPart(filename, fat_start+fat_size , fat_start + fat_size*2);

        allFiles = getFilesFromRootDirectory(rootDirectory);

        allFiles = collectEntries(std::ref(allFiles), std::ref(fat), std::ref(fat2), std::ref(filename), root_folder_loc, root_size);
//
//        for(auto &sss:allFiles) getFatChain(std::ref(fat), std::ref(fat2), std::ref(sss.fat_chain));
//
//        copyClusters(std::ref(allFiles), std::ref(filename), root_folder_loc, root_size, fat_start, fat_size);
//
//        rootDirectory = readPart(filename, root_folder_loc, root_folder_loc + root_size);
//        fat = readPart(filename, fat_start , fat_start + fat_size);
//        fat2 = readPart(filename, fat_start+fat_size , fat_start + fat_size*2);
//
//        allFiles = getFilesFromRootDirectory(rootDirectory);
//        allFiles = collectEntries(std::ref(allFiles), std::ref(fat), std::ref(fat2), std::ref(filename), root_folder_loc, root_size);

//        for(auto &sss:allFiles) getFatChain(std::ref(fat), std::ref(fat2), std::ref(sss.fat));ss


//        storeData(std::ref(allFiles), std::ref(fat), std::ref(fat2), root_folder_loc, root_size, std::ref(filename),"root", std::ref(chains), true, fat_start);


    }
}


