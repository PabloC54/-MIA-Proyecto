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

bool logged;
string user_logged;
const char *disk_path;
string partition_name;

int login(string usuario, string password, string id)
{
    if (logged)
        throw Exception("session already active; logout before logging in");

    if (id.length() < 4 || id.length() > 5)
        throw Exception("non-valid id");
    if (id.substr(0, 2) != "98")
        throw Exception("non-valid id (ids start with '98')");

    const char *path;
    string name;
    vector<const char *> data;

    if (id.length() == 4)
        data = getPartitionMountedByID(id.substr(3, 4)[0], stoi(id.substr(2, 3)));
    else
        data = getPartitionMountedByID(id.substr(4, 5)[0], stoi(id.substr(2, 4)));

    if (data.empty())
        throw Exception("id does not exist");

    path = data[1];
    name = data[0];

    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(&mbr, name);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    inode inodo;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[16];
    strftime(date, 16, "%d/%m/%Y %H:%M", tm);
    strcpy(inodo.atime, date);

    string content = "";

    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content).substr(0, sizeof(file_block));
    }

    stringstream ss(content);
    string line;
    string group;
    vector<vector<string>> groups;

    while (getline(ss, line, '\n'))
    {
        stringstream us(line);
        string substr;
        vector<string> words;

        while (getline(us, substr, ','))
        {
            words.push_back(substr);
        }

        if (words.at(1) == "G")
            groups.push_back(words);

        if (words.at(1) == "U")
            if (words.at(3) == usuario)
            {
                if (words.at(0) == "0")
                    throw Exception("non-existent user");

                group = words.at(2);

                if (words.at(4) != password)
                {
                    fclose(file);
                    throw Exception("wrong password");
                }
            }
    }

    for (vector<string> grp : groups)
        if (grp.at(2) == group && grp.at(0) == "0")
            throw Exception("non-existent user");

    logged = true;
    user_logged = usuario;
    disk_path = path;
    partition_name = name;

    fclose(file);

    return 0;
}

int logout()
{
    if (!logged)
        throw Exception("no session active");

    logged = false;

    return 0;
}

int mkgrp(string name)
{
    if (!logged)
        throw Exception("no session active");

    if (user_logged != "root")
        throw Exception("user has no permissions");

    if (name.length() > 10)
    {
        name = name.substr(0, 10);
        cout << "name truncated to '" << name << "'" << endl;
    }

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

    inode inodo;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    string content = "";

    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content).substr(0, sizeof(file_block));
    }

    stringstream ss(content);
    string line;
    int new_index = 1;

    try
    {
        while (getline(ss, line, '\n'))
        {
            stringstream us(line);
            string substr;
            vector<string> words;

            while (getline(us, substr, ','))
            {
                words.push_back(substr);
            }

            if (words.at(1) == "G")
            {
                new_index += 1;
                if (words.at(2) == name)
                    throw Exception("already existent group");
            }
        }

        content += std::to_string(new_index) + ",G," + name + "\n";
        string temp;
        bool b_break = false;

        file = fopen(disk_path, "r+b");

        for (int i = 0; i < 12; i++)
        {
            if (((i + 1) * 64) >= content.length())
            {
                temp = content.substr(i * 64, content.length());
                b_break = true;
            }
            else
            {
                temp = content.substr(i * 64, (i + 1) * 64);
            }

            file_block block;
            strcpy(block.content, temp.c_str());

            if (inodo.block[i] == -1)
            {
                inodo.block[i] = super.first_block;
                super.free_blocks_count -= 1;
                super.first_block += 1;
            }

            fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
            fwrite(&block, sizeof(file_block), 1, file);

            if (b_break)
                break;
        }
    }
    catch (const Exception &e)
    {
        strcpy(inodo.atime, date);
        fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);

        strcpy(super.umtime, date);
        fseek(file, partition->start, SEEK_SET);
        fwrite(&super, sizeof(superblock), 1, file);

        throw Exception(e.what());
    }

    strcpy(inodo.mtime, date);
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strcpy(super.mtime, date);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int rmgrp(string name)
{
    if (!logged)
        throw Exception("no session active");

    if (user_logged != "root")
        throw Exception("user has no permissions");

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

    inode inodo;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    string content = "";

    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content).substr(0, sizeof(file_block));
    }

    stringstream ss(content);
    string line;
    string new_content = "";

    try
    {

        while (getline(ss, line, '\n'))
        {
            stringstream us(line);
            string substr;
            vector<string> words;

            while (getline(us, substr, ','))
            {
                words.push_back(substr);
            }

            if (words.at(1) == "G")
            {
                if (words.at(2) == name)
                {
                    new_content += "0,G," + words.at(2) + "\n";
                    continue;
                }
            }

            new_content += line + "\n";
        }

        string temp;
        bool b_break = false;

        file = fopen(disk_path, "r+b");

        for (int i = 0; i < 12; i++)
        {
            if (((i + 1) * 64) >= new_content.length())
            {
                temp = new_content.substr(i * 64, new_content.length());
                b_break = true;
            }
            else
            {
                temp = new_content.substr(i * 64, (i + 1) * 64);
            }

            file_block block;
            strcpy(block.content, temp.c_str());

            if (inodo.block[i] == -1)
            {
                inodo.block[i] = super.first_block;
                super.free_blocks_count -= 1;
                super.first_block += 1;
            }

            fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
            fwrite(&block, sizeof(file_block), 1, file);

            if (b_break)
                break;
        }
    }
    catch (const Exception &e)
    {
        strcpy(inodo.atime, date);
        fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);

        strcpy(super.umtime, date);
        fseek(file, partition->start, SEEK_SET);
        fwrite(&super, sizeof(superblock), 1, file);

        throw Exception(e.what());
    }

    strcpy(inodo.mtime, date);
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strcpy(super.mtime, date);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int mkusr(string usr, string pwd, string grp)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user_logged != "root")
        throw Exception("user has no permissions");

    if (usr.length() > 10)
    {
        usr = usr.substr(0, 10);
        cout << "usr truncated to '" << usr << "'" << endl;
    }
    if (pwd.length() > 10)
    {
        pwd = pwd.substr(0, 10);
        cout << "pwd truncated to '" << pwd << "'" << endl;
    }
    if (grp.length() > 10)
    {
        grp = grp.substr(0, 10);
        cout << "grp truncated to '" << grp << "'" << endl;
    }

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

    inode inodo;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    string content = "";

    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content).substr(0, sizeof(file_block));
    }

    stringstream ss(content);
    string line;
    int new_index = 1;

    bool existent_grp = false;

    try
    {
        while (getline(ss, line, '\n'))
        {
            stringstream us(line);
            string substr;
            vector<string> words;

            while (getline(us, substr, ','))
            {
                words.push_back(substr);
            }

            if (words.at(0) != "0" && words.at(1) == "G" && words.at(2) == grp)
                existent_grp = true;

            if (words.at(1) == "U")
            {
                new_index += 1;
                if (words.at(2) == grp && words.at(3) == usr)
                    throw Exception("already existent user");
            }
        }

        if (!existent_grp)
            throw Exception("non-existent group");

        content += std::to_string(new_index) + ",U," + grp + "," + usr + "," + pwd + "\n";
        string temp;
        bool b_break = false;

        file = fopen(disk_path, "r+b");

        for (int i = 0; i < 12; i++)
        {
            if (((i + 1) * sizeof(file_block)) >= content.length())
            {
                temp = content.substr(i * sizeof(file_block), content.length());
                b_break = true;
            }
            else
            {
                temp = content.substr(i * sizeof(file_block), (i + 1) * sizeof(file_block));
            }

            file_block block;
            for (int i = 0; i < 64; i++)
                block.content[i] = '\0';

            strcpy(block.content, temp.c_str());

            if (inodo.block[i] == -1)
            {
                inodo.block[i] = super.first_block;
                super.free_blocks_count -= 1;
                super.first_block += 1;
            }

            fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
            fwrite(&block, sizeof(file_block), 1, file);

            if (b_break)
                break;
        }
    }
    catch (const Exception &e)
    {
        strcpy(inodo.atime, date);
        fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);

        strcpy(super.umtime, date);
        fseek(file, partition->start, SEEK_SET);
        fwrite(&super, sizeof(superblock), 1, file);

        throw Exception(e.what());
    }

    strcpy(inodo.mtime, date);
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strcpy(super.mtime, date);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int rmusr(string usr)
{
    if (!logged)
        throw Exception("no session active");

    if (user_logged != "root")
        throw Exception("user has no permissions");

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

    inode inodo;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    string content = "";

    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content).substr(0, sizeof(file_block));
    }

    stringstream ss(content);
    string line;
    string new_content = "";

    try
    {
        while (getline(ss, line, '\n'))
        {
            stringstream us(line);
            string substr;
            vector<string> words;

            while (getline(us, substr, ','))
            {
                words.push_back(substr);
            }

            if (words.at(1) == "U")
            {
                if (words.at(3) == usr)
                {
                    new_content += "0,U," + words.at(2) + "," + words.at(3) + "," + words.at(4) + "\n";
                    continue;
                }
            }

            new_content += line + "\n";
        }

        string temp;
        bool b_break = false;

        file = fopen(disk_path, "r+b");

        for (int i = 0; i < 12; i++)
        {
            if (((i + 1) * 64) >= new_content.length())
            {
                temp = new_content.substr(i * 64, new_content.length());
                b_break = true;
            }
            else
            {
                temp = new_content.substr(i * 64, (i + 1) * 64);
            }

            file_block block;
            strcpy(block.content, temp.c_str());

            if (inodo.block[i] == -1)
            {
                inodo.block[i] = super.first_block;
                super.free_blocks_count -= 1;
                super.first_block += 1;
            }

            fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
            fwrite(&block, sizeof(file_block), 1, file);

            if (b_break)
                break;
        }
    }
    catch (const Exception &e)
    {
        strcpy(inodo.atime, date);
        fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);

        strcpy(super.umtime, date);
        fseek(file, partition->start, SEEK_SET);
        fwrite(&super, sizeof(superblock), 1, file);

        throw Exception(e.what());
    }

    strcpy(inodo.mtime, date);
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strcpy(super.mtime, date);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}
