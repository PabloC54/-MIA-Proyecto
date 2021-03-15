#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>
#include <regex>
#include <boost/algorithm/string.hpp>

#include "usermanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"
#include "../Util/util.h"

using namespace std;

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

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

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

        strncpy(inodo.atime, date, 16);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (r.empty())
    {
        inodo.permissions = stoi(ugo);
        strncpy(inodo.mtime, date, 16);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);
    }
    else
    {
        if (inodo.type == 1)
            cout << "recursive permission change cannot be applied on files";

        change_permission(file, super, num_nodo, ugo, date);
    }

    strncpy(super.umtime, date, 16);
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

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    string filename = words.at(words.size() - 1);
    words.pop_back();

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    bool exists;
    string folder = "/";

    for (string dir : words)
    {
        exists = exists_inode(file, &super, &inodo, dir);

        if (exists)
        {
            num_nodo = get_inode(file, &super, &inodo, dir);
        }
        else
        {
            if (r.empty())
            {
                string msg = "'" + dir + "' not found";
                throw Exception(msg.c_str());
            }
            else
            {
                if (!has_permission(inodo, "w"))
                {
                    string msg = "'" + folder + "' write access denied";
                    throw Exception(msg.c_str());
                }

                num_nodo = new_folder(file, &super, &inodo, num_nodo, dir, date);
            }
        }

        folder = dir;

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);

        if (inodo.type == '\1')
            throw Exception("a folder path is required");
    }

    if (!has_permission(inodo, "w"))
    {
        string msg = "'" + folder + "' write access denied";
        throw Exception(msg.c_str());
    }

    int new_num = new_file(file, &super, &inodo, num_nodo, filename, date);

    inode new_inodo;
    strncpy(new_inodo.ctime, date, 16);
    strncpy(new_inodo.mtime, date, 16);
    new_inodo.type = '\1';
    new_inodo.permissions = inodo.permissions;
    new_inodo.uid = user_num;
    new_inodo.gid = group_num;

    string content = "", temp;

    if (cont.empty())
    {
        int size_int;
        if (!size.empty())
            size_int = stoi(size);
        else
            size_int = 0;

        for (int i = 0; i < size_int; i += 10)
        {
            if (i + 10 < size_int)
                content += "0123456789";
            else
                content += string("0123456789").substr(0, size_int - i);
        }

        create_journal(file, &super, date, "mkfile", path, "", size_int, '\1');
    }
    else
    {
        ifstream data(cont);
        if (!data)
        {
            string msg = "'" + cont + "' not found";
            throw Exception(msg.c_str());
        }

        string line;
        while (!data.eof())
            while (getline(data, line))
                content += line + "\n";

        create_journal(file, &super, date, "mkfile", path, cont, 0, '\1');
    }

    write_inode_content(file, &super, &new_inodo, content);

    fseek(file, super.inode_start + new_num * sizeof(inode), SEEK_SET);
    fwrite(&new_inodo, sizeof(inode), 1, file);

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int cat(map<string, string> filen)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    string param, path;
    string full_content = "";

    map<string, string>::iterator it;
    for (it = filen.begin(); it != filen.end(); ++it)
    {
        param = it->first;
        path = it->second;

        stringstream ss(path.substr(1, path.size() - 1));

        string dir;
        vector<string> words;
        while (getline(ss, dir, '/'))
        {
            words.push_back(dir);
        }
        string file_name = words.at(words.size() - 1);

        inode inodo;
        fseek(file, super.inode_start, SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);

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
                string msg = "'" + path + "' not found";
                throw Exception(msg.c_str());
            }

            fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
            fread(&inodo, sizeof(inode), 1, file);
        }

        if (inodo.type != '\1')
        {
            string msg = "'" + path + "' not a file";
            throw Exception(msg.c_str());
        }

        string content = "";
        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            file_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
            fread(&block, sizeof(file_block), 1, file);

            content += string(block.content).substr(0, 64);
        }

        if (!(has_permission(inodo, "r")))
        {
            string msg = "'" + path + "' read acces denied";
            throw Exception(msg.c_str());
        }

        full_content += "\033[1;36m" + file_name + "\033[0m\n";
        full_content += content;
        full_content += "\n";
    }

    cout << endl
         << full_content;

    return 0;
}

int rem(string path)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    string dirname = words.at(words.size() - 1);

    file = fopen(disk_path, "r+b");

    inode inodo, last_inodo;
    int num_nodo, num_last_nodo;

    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);
    strncpy(inodo.atime, date, 16);

    bool found = false;

    for (string dir : words)
    {
        last_inodo = inodo;
        num_last_nodo = num_nodo;

        if (inodo.type == '\1')
            throw Exception("a folder path is required");

        num_nodo = get_inode(file, &super, &inodo, dir);
        found = num_nodo != 0;

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
        strncpy(inodo.atime, date, 16);
    }

    if (!has_permission(inodo, "w"))
    {
        string msg = "write access denied";
        throw Exception(msg.c_str());
    }

    if (!is_deleteable(file, super, inodo))
        throw Exception("could not delete one of more contents");

    delete_inode(file, super, inodo);

    found = false;
    for (int i = 0; i < 12; i++)
    {
        if (last_inodo.block[i] == -1)
            continue;

        folder_block block;
        fseek(file, super.block_start + last_inodo.block[i] * sizeof(folder_block), SEEK_SET);
        fread(&block, sizeof(folder_block), 1, file);

        for (int j = 0; j < 4; j++)
            if (block.content[j].name == dirname)
            {
                block.content[j].inode = -1;
                strncpy(block.content[j].name, "\\\\\\\\\\\\\\\\\\\\\\\\", 12);
                fseek(file, super.block_start + last_inodo.block[i] * sizeof(folder_block), SEEK_SET);
                fwrite(&block, sizeof(folder_block), 1, file);
                break;
            }

        if (found)
            break;
    }

    folder_block block;

    fseek(file, super.bm_inode_start + num_nodo, SEEK_SET);
    fwrite("\0", 1, 1, file);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite("\0", sizeof(inode), 1, file);

    super.first_block = get_first_block(file, super);
    super.first_inode = get_first_inode(file, super);

    strncpy(inodo.mtime, date, 16);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int edit(string path, string cont)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    bool found;
    string folder = "/";

    for (string dir : words)
    {
        if (inodo.type == '\1')
            throw Exception("a folder path is required");

        num_nodo = get_inode(file, &super, &inodo, dir);
        found = num_nodo != 0;

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        folder = dir;

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (inodo.type == '\0')
        throw Exception("cannot edit a folder");

    if (!has_permission(inodo, "w") || !has_permission(inodo, "r"))
    {
        string msg = "'" + folder + "' write/read access denied";
        throw Exception(msg.c_str());
    }

    ifstream data(cont);
    if (!data)
    {
        string msg = "'" + cont + "' not found";
        throw Exception(msg.c_str());
    }

    string content = "", line;
    while (!data.eof())
        while (getline(data, line))
            content += line + "\n";

    write_inode_content(file, &super, &inodo, content);

    strncpy(inodo.mtime, date, 16);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int ren(string path, string name)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    string filename = words.at(words.size() - 1);

    inode inodo, last_inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    bool found;
    string folder = "/";

    for (string dir : words)
    {
        if (inodo.type == '\1')
            throw Exception("a folder path is required");

        last_inodo = inodo;
        num_nodo = get_inode(file, &super, &inodo, dir);
        found = num_nodo != 0;

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        folder = dir;

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (!has_permission(inodo, "w"))
    {
        string msg = "'" + folder + "' write/read access denied";
        throw Exception(msg.c_str());
    }

    if (get_inode(file, &super, &last_inodo, name) != 0)
    {
        string msg = "'" + name + "' already existent in this directory";
        throw Exception(msg.c_str());
    }

    found = false;
    for (int i = 0; i < 12; i++)
    {
        if (last_inodo.block[i] == -1)
            continue;

        folder_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
        fread(&block, sizeof(folder_block), 1, file);

        for (int j = 0; j < 4; j++)
            if (block.content[j].name == filename)
            {
                strncpy(block.content[j].name, name.c_str(), 12);
                fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
                fwrite(&block, sizeof(folder_block), 1, file);
                break;
            }

        if (found)
            break;
    }

    strncpy(inodo.mtime, date, 16);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int mkdir(string path, string p)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    string filename = words.at(words.size() - 1);
    words.pop_back();

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    bool exists;
    string folder = "/";

    for (string dir : words)
    {
        exists = exists_inode(file, &super, &inodo, dir);

        if (exists)
        {
            num_nodo = get_inode(file, &super, &inodo, dir);
        }
        else
        {
            if (p.empty())
            {
                string msg = "'" + dir + "' not found";
                throw Exception(msg.c_str());
            }
            else
            {
                if (!has_permission(inodo, "w"))
                {
                    string msg = "'" + folder + "' write access denied";
                    throw Exception(msg.c_str());
                }

                num_nodo = new_folder(file, &super, &inodo, num_nodo, dir, date);
            }
        }

        folder = dir;

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);

        if (inodo.type == '\1')
            throw Exception("a folder path is required");
    }

    if (!has_permission(inodo, "w"))
    {
        string msg = "'" + folder + "' write access denied";
        throw Exception(msg.c_str());
    }

    int new_num = new_folder(file, &super, &inodo, num_nodo, filename, date);

    inode new_inodo;
    fseek(file, super.inode_start + new_num * sizeof(inode), SEEK_SET);
    fread(&new_inodo, sizeof(inode), 1, file);

    strncpy(new_inodo.ctime, date, 16);
    strncpy(new_inodo.mtime, date, 16);
    new_inodo.uid = user_num;
    new_inodo.gid = group_num;
    fseek(file, super.inode_start + new_num * sizeof(inode), SEEK_SET);
    fwrite(&new_inodo, sizeof(inode), 1, file);

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int cp(string path, string dest)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    string filename = words.at(words.size() - 1);

    inode inodo, dest_inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);
    fseek(file, super.inode_start, SEEK_SET);
    fread(&dest_inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    bool found;
    string folder = "/";

    for (string dir : words)
    {
        if (inodo.type == '\1')
            throw Exception("a folder path is required");

        num_nodo = get_inode(file, &super, &inodo, dir);
        found = num_nodo != 0;

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        folder = dir;

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (!has_permission(inodo, "r"))
    {
        string msg = "'" + folder + "' read access denied";
        throw Exception(msg.c_str());
    }

    // consiguiendo el nuevo destino
    stringstream sss(dest.substr(1, dest.length()));
    words.clear();

    while (getline(sss, dir, '/'))
        words.push_back(dir);

    for (string dir : words)
    {
        num_nodo = get_inode(file, &super, &dest_inodo, dir);
        found = num_nodo != 0;

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        folder = dir;

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&dest_inodo, sizeof(inode), 1, file);

        if (dest_inodo.type == '\1')
            throw Exception("a folder destination path is required");
    }

    if (get_inode(file, &super, &inodo, filename) != 0)
    {
        string msg = "'" + filename + "' already existent in destination directory";
        throw Exception(msg.c_str());
    }

    // referencia desde nodo destino al nuevo inodo copia

    copy_inode(file, &super, inodo);

    strncpy(inodo.mtime, date, 16);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int mv(string path, string mv)
{

    return 0;
}

int find(string path, string name)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    stringstream ss(path.substr(1, path.length()));

    vector<string> words;
    string dir;
    while (getline(ss, dir, '/'))
        words.push_back(dir);

    string dirname = "/";
    if (!words.empty())
        dirname += words.at(words.size() - 1);

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    bool found;
    string folder = "/";

    for (string dir : words)
    {
        num_nodo = get_inode(file, &super, &inodo, dir);
        found = num_nodo != 0;

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        folder = dir;

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);

        if (inodo.type == '\1')
            throw Exception("a folder path is required");
    }

    if (!has_permission(inodo, "r"))
    {
        string msg = "'" + folder + "' read access denied";
        throw Exception(msg.c_str());
    }

    name = "^" + name + "$";

    string pt = "\\.";

    size_t pos = 0;
    while ((pos = name.find(".", pos)) != std::string::npos)
    {
        name.replace(pos, 1, pt.c_str());
        pos += 2;
    }
    pos = 0;
    while ((pos = name.find("\\", pos)) != std::string::npos)
    {
        name.replace(pos, 1, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = name.find("?", pos)) != std::string::npos)
    {
        name.replace(pos, 1, "[^.]");
        pos += 4;
    }
    pos = 0;
    while ((pos = name.find("*", pos)) != std::string::npos)
    {
        name.replace(pos, 1, "[^.]+");
        pos += 5;
    }

    regex f(name.c_str());

    string s = folder + "\n";

    s += find_inode(file, super, inodo, folder, 0, f);

    cout << endl
         << s << endl;

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int chown(string path, string r, string usr)
{
    if (!logged)
        throw Exception("no sesion active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);

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

        strncpy(inodo.atime, date, 16);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (user_logged != "root" && user_num != inodo.uid)
        throw Exception("user has no permissions");

    // buscar el uid del nuevo usuario usr

    inode inodo_users;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo_users, sizeof(inode), 1, file);
    strncpy(inodo.atime, date, 16);

    string content = read_inode_content(file, super, inodo_users);

    string line, group;
    int new_uid;
    bool found = false;
    vector<vector<string>> groups;

    while (getline(ss, line, '\n'))
    {
        stringstream us(line);
        string substr;
        vector<string> words;

        while (getline(us, substr, ','))
            words.push_back(substr);

        if (words.at(1) == "G")
            groups.push_back(words);

        if (words.at(1) == "U")
            if (words.at(3) == usr)
            {
                if (words.at(0) == "0")
                    throw Exception("non-existent user");

                new_uid = stoi(words.at(0));
                group = words.at(2);
            }
    }

    if (!found)
        throw Exception("non-existent user");

    for (vector<string> grp : groups)
        if (grp.at(2) == group)
            if (grp.at(0) == "0")
                throw Exception("non-existent user");

    if (r.empty())
    {
        inodo.uid = new_uid;
        strncpy(inodo.mtime, date, 16);
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fwrite(&inodo, sizeof(inode), 1, file);
    }
    else
    {
        if (inodo.type == 1)
            cout << "recursive permission change cannot be applied on files";

        change_owner(file, super, num_nodo, new_uid, date);
    }

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

    return 0;
}

int chgrp(string usr, string grp)
{
    if (!logged)
        throw Exception("no session active");

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    inode inodo;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[17];
    strftime(date, 17, "%d/%m/%Y %H:%M", tm);
    strncpy(inodo.atime, date, 16);

    string content = read_inode_content(file, super, inodo);

    stringstream ss(content);
    string line, group;
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
            if (words.at(3) == usr)
            {
                if (words.at(0) == "0")
                    throw Exception("non-existent user");

                group = words.at(2);
            }
    }

    bool group_found = false;

    for (vector<string> temp_grp : groups)
    {
        if (temp_grp.at(2) == group)
        {
            if (temp_grp.at(0) == "0")
                throw Exception("non-existent user");
        }

        if (temp_grp.at(2) == grp)
        {
            if (temp_grp.at(0) == "0")
                throw Exception("non-existent group");

            group_found = true;
        }
    }

    if (!group_found)
        throw Exception("non-existent group");

    string new_content = "";

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
                new_content += words.at(0) + ",U," + group + "," + words.at(3) + "," + words.at(4) + "\n";
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
            fseek(file, super.bm_block_start + super.first_block * sizeof(file_block), SEEK_SET);
            fwrite("\1", 1, 1, file);

            super.free_blocks_count -= 1;
            super.first_block = get_first_block(file, super);
        }

        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fwrite(&block, sizeof(file_block), 1, file);

        if (b_break)
            break;
    }

    return 0;
}
