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

int change_permission(FILE *file, superblock super, int num_nodo, string ugo, char *date)
{
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    inode inodo;
    fread(&inodo, sizeof(inode), 1, file);
    inodo.permissions = stoi(ugo);
    strcpy(inodo.mtime, date);

    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    if (inodo.type == 0)
    {
        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            folder_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
            {
                if (block.content[j].inode == -1 || string(block.content[j].name) == "." || string(block.content[j].name) == "..")
                    continue;

                change_permission(file, super, block.content[j].inode, ugo, date);
            }
        }
    }

    return 0;
}

bool has_permission(inode inodo, const char *access)
{
    /*
    r w x
    1 0 1
    */

    string temp = to_string(inodo.permissions);
    int permission;

    if (inodo.uid == user_num)
        permission = stoi(temp.substr(0, 1));
    else if (inodo.gid == user_num)
        permission = stoi(temp.substr(1, 2));
    else
        permission = stoi(temp.substr(2, 3));

    int x = permission % 2;
    int r = (permission / 2) % 2;
    int w = (permission / 4) % 2;

    if (access == "r")
        if (r == 4 || r == 5 || r == 6 || r == 7)
            return true;
    if (access == "w")
        if (w == 2 || w == 6 || w == 7)
            return true;
    if (access == "x")
        if (x == 1 || x == 3 || x == 5 || x == 7)
            return true;

    return false;
}

int chmod(string path, string ugo, string r)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user_logged != "root")
        throw Exception("user has no permissions");

    if (ugo.length() != 3)
        throw Exception("-ugo parameter non-valid");

    if (stoi(ugo) > 777 || stoi(ugo) / 10 > 77 || stoi(ugo) / 100 > 7)
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

            if (found)
                break;
        }

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        strcpy(inodo.atime, date);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (r.empty())
    {
        inodo.permissions = stoi(ugo);
        strcpy(inodo.mtime, date);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);
    }
    else
    {
        if (inodo.type == 1)
            cout << "recursive permission change cannot be applied on files";

        change_permission(file, super, num_nodo, ugo, date);
    }

    strcpy(super.umtime, date);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int mkfile(string path, string r, string size, string cont)
{
    if (!logged)
        throw Exception("no sesion active");

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

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    string new_file = words.at(words.size() - 1);
    words.pop_back();

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;

    for (string dir : words)
    {
        bool found = false;

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

            if (found)
                break;
        }

        if (!found)
        {
            if (r.empty())
            {
                string msg = "'" + dir + "' not found";
                throw Exception(msg.c_str());
            }
            else
            {
                // crear carpeta nueva
            }
        }

        strcpy(inodo.atime, date);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);

        if (inodo.type == '\1')
            throw Exception("a folder path is required");
    }

    if (!has_permission(inodo, "w"))
        throw Exception("access denied");

    for (int i = 0; i < 12; i++)
    {
        folder_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);

        if (inodo.block[i] != -1)
            fread(&block, sizeof(folder_block), 1, file);
        else
        {
            inodo.block[i] = super.first_block;
            super.free_blocks_count -= 1;
            super.first_block += 1;
        }

        for (int j = 0; j < 4; j++)
        {
            if (block.content[j].name == new_file)
            {
                cout << "already existent file. replace? (y/n)" << endl;

                string rep;
                cin >> rep;

                if (rep == "n" || rep == "N")
                    return 0;
            }
            else if (block.content[j].inode != -1)
                continue;

            strcpy(block.content[j].name, new_file.c_str());
            block.content[j].inode = super.first_inode;

            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fwrite(&block, sizeof(folder_block), 1, file);

            inode inodo_f;
            strcpy(inodo_f.ctime, date);
            inodo_f.type = '\1';
            inodo_f.permissions = inodo.permissions;

            if (cont.empty())
            {
                int size_int;

                if (!size.empty())
                    size_int = stoi(size);
                else
                    size_int = 0;

                string content = "";

                for (int i = 0; i < size_int; i += 10)
                {
                    if (i + 10 < size_int)
                        content += "0123456789";
                    else
                        content += string("0123456789").substr(0, 666);
                }

                if (size_int == 0)
                {
                    file_block block;
                    inodo_f.block[0] = super.first_block;
                    super.first_block += 1;
                    super.free_blocks_count -= 1;
                }

                for (int i = 0; i < size_int; i += 64)
                {
                    // if ((i + 1) * 64 > size_int)
                }
            }
            else
            {
            }
        }
    }

    // if (inodo.)
    //     inodo.permissions = stoi(ugo);
    strcpy(inodo.mtime, date);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strcpy(super.umtime, date);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

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
