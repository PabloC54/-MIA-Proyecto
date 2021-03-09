#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "usermanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"
#include "../Util/util.h"

using namespace std;

// bool logged;
// string user_logged;
// const char *disk_path;
// string partition_name;

int change_permission(FILE file, superblock super, inode inodo, int num_nodo, string ugo, char *date)
{
    inodo.permissions = stoi(ugo);
    strcpy(inodo.mtime, date);
    fseek(&file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, &file);

    if (inodo.type == 0)
    {
        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            folder_block block;
            fseek(&file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, &file);

            for (int j = 0; j < 4; j++)
            {
                if (block.content[j].name == "." || block.content[j].name == "..")
                    continue;

                fseek(&file, super.inode_start + block.content[j].inode * sizeof(inode), SEEK_SET);
                fread(&inodo, sizeof(inode), 1, &file);

                change_permission(file, super, inodo, block.content[j].inode, ugo, date);
            }
        }
    }

    return 0;
}

int chmod(string path, string ugo, string r)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user_logged != "root")
        throw Exception("user has no permissions");

    if (ugo.length() != 3)
        throw Exception("-ugo parameter non-valid");

    if (stoi(ugo.substr(0, 1)) > 7 || stoi(ugo.substr(1, 2)) > 7 || stoi(ugo.substr(2, 3)) > 7)
        throw Exception("-ugo parameter non-valid");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(&mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[16];
    strftime(date, 16, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));
    string dir;

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;

    while (getline(ss, dir, '/'))
    {
        bool found = false;

        strcpy(inodo.atime, date);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);

        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            folder_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
            {
                if (block.content[j].name == dir)
                {
                    num_nodo = block.content[j].inode;
                    found = true;
                }
            }
        }

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }
    }

    if (r != "true")
    {
        inodo.permissions = stoi(ugo);
        strcpy(inodo.mtime, date);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);
    }
    else
    {
        if (inodo.type == 1)
            cout << "recursive permission change cannot be applied on files  ";

        change_permission(*file, super, inodo, num_nodo, ugo, date);
    }

    strcpy(super.umtime, date);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int mkfile(string path, string r, string size, string cont)
{

    return 0;
}

int cat(string filen)
{

    return 0;
}

int rem(string path)
{

    return 0;
}

int edit(string path, string cont)
{

    return 0;
}

int ren(string path, string name)
{

    return 0;
}

int mkdir(string path, string p)
{

    return 0;
}

int cp(string path, string dest)
{

    return 0;
}

int mv(string path, string mv)
{

    return 0;
}

int find(string path, string name)
{

    return 0;
}

int chown(string path, string r, string usr)
{

    return 0;
}

int chgrp(string usr, string grp)
{

    return 0;
}
