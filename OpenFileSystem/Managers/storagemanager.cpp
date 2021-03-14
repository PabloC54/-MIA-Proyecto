#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>

#include "usermanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"
#include "../Util/util.h"

using namespace std;

int create_journal(FILE *file, superblock *super, const char *date, string operation, string path, string content, int size, char type)
{
    if (super->filesystem_type != 3)
        return 0;

    journal j;
    strncpy(j.date, date, 16);
    strncpy(j.operation, operation.c_str(), 10);
    strncpy(j.content, content.c_str(), 50);
    strncpy(j.path, path.c_str(), 100);
    j.size = size;
    j.type = type;

    fseek(file, super->first_journal, SEEK_SET);
    fwrite(&j, sizeof(journal), 1, file);
    super->first_journal += 1;
    super->free_journal_count -= 1;

    return 0;
}

int get_first_inode(FILE *file, superblock super)
{
    char b;
    int value = -1;

    fseek(file, super.bm_inode_start, SEEK_SET);
    for (int i = 0; i < super.inodes_count; i++)
    {
        fread(&b, 1, 1, file);

        if (b == '\0')
        {
            value = i;
            break;
        }
    }
    if (value == -1)
        cout << "no free inodes" << endl;

    return value;
}

int get_first_block(FILE *file, superblock super)
{
    char b;
    int value = -1;

    fseek(file, super.bm_block_start, SEEK_SET);
    for (int i = 0; i < super.blocks_count; i++)
    {
        fread(&b, 1, 1, file);

        if (b == '\0')
        {
            value = i;
            break;
        }
    }
    if (value == -1)
        cout << "no free blocks" << endl;

    return value;
}

bool has_permission(inode inodo, const char *access)
{
    string temp = to_string(inodo.permissions);
    int permission;

    if (inodo.uid == user_num)
        permission = stoi(temp.substr(0, 1));
    else if (inodo.gid == user_num)
        permission = stoi(temp.substr(1, 1));
    else
        permission = stoi(temp.substr(2, 1));

    int x = permission % 2;
    int w = (permission / 2) % 2;
    int r = (permission / 4) % 2;

    if (access == "r")
        if (r == 1)
            return true;
    if (access == "w")
        if (w == 1)
            return true;
    if (access == "x")
        if (x == 1)
            return true;

    return false;
}

int change_permission(FILE *file, superblock super, int num_nodo, string ugo, const char *date)
{
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    inode inodo;
    fread(&inodo, sizeof(inode), 1, file);
    inodo.permissions = stoi(ugo);
    strncpy(inodo.mtime, date, 16);

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

int change_owner(FILE *file, superblock super, int num_nodo, int uid, const char *date)
{
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    inode inodo;
    fread(&inodo, sizeof(inode), 1, file);
    inodo.uid = uid;
    strncpy(inodo.mtime, date, 16);

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

                change_owner(file, super, block.content[j].inode, uid, date);
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

    string new_file = words.at(words.size() - 1);
    words.pop_back();

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    string folder = "/";

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
                if (!has_permission(inodo, "w"))
                {
                    string msg = "'" + folder + "' write access denied";
                    throw Exception(msg.c_str());
                }

                bool space_found = false;
                for (int j = 0; j < 12; j++)
                {
                    folder_block bloque_temp;

                    if (inodo.block[j] == -1)
                    {
                        inodo.block[j] = super.first_block;
                        fseek(file, super.bm_block_start + super.first_block, SEEK_SET);
                        fwrite("\1", 1, 1, file);
                        super.free_blocks_count -= 1;
                        super.first_block = get_first_block(file, super);

                        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
                        fwrite(&inodo, sizeof(inode), 1, file);
                    }
                    else
                    {
                        fseek(file, super.block_start + inodo.block[j] * sizeof(folder_block), SEEK_SET);
                        fread(&bloque_temp, sizeof(folder_block), 1, file);
                    }

                    for (int k = 0; k < 4; k++)
                    {
                        if (bloque_temp.content[k].inode == -1)
                        {
                            bloque_temp.content[k].inode = super.first_inode;
                            strncpy(bloque_temp.content[k].name, dir.c_str(), 12);
                            fseek(file, super.block_start + inodo.block[j] * sizeof(folder_block), SEEK_SET);
                            fwrite(&bloque_temp, sizeof(folder_block), 1, file);

                            space_found = true;
                            break;
                        }
                    }

                    if (space_found)
                        break;
                }

                // nuevo inodo carpeta, bloque del inodo carpeta

                inode new_inodo;
                new_inodo.block[0] = super.first_block;
                strncpy(new_inodo.ctime, date, 16);

                folder_block bloque;
                strcpy(bloque.content[0].name, ".");
                bloque.content[0].inode = super.first_inode;
                strcpy(bloque.content[1].name, "..");
                bloque.content[1].inode = num_nodo;

                fseek(file, super.bm_inode_start + super.first_inode, SEEK_SET);
                fwrite("\1", 1, 1, file);
                fseek(file, super.inode_start + super.first_inode * sizeof(inode), SEEK_SET);
                fwrite(&new_inodo, sizeof(inode), 1, file);

                fseek(file, super.bm_block_start + super.first_block, SEEK_SET);
                fwrite("\1", 1, 1, file);
                fseek(file, super.block_start + super.first_block * sizeof(folder_block), SEEK_SET);
                fwrite(&bloque, sizeof(folder_block), 1, file);

                num_nodo = super.first_inode;

                super.free_inodes_count -= 1;
                super.first_inode = get_first_inode(file, super);
                super.free_blocks_count -= 1;
                super.first_block = get_first_block(file, super);
            }
        }

        folder = dir;

        strncpy(inodo.atime, date, 16);
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

    bool found = false, replace = false;
    int num_block, num_content;
    folder_block block;

    // buscando espacio en la ultima carpeta
    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] != -1)
        {
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
            {
                if (block.content[j].name == new_file)
                {
                    cout << "already existent file. replace? (y/n)" << endl;

                    string rep;
                    cin >> rep;

                    if (rep == "n" || rep == "N")
                        return 0;

                    replace = true;
                }
                else if (block.content[j].inode != -1)
                    continue;

                num_block = i;
                num_content = j;
                found = true;
                break;
            }
        }
        else
        {
            for (int j = 0; j < 4; j++)
            {
                block.content[j].inode = -1;
                strncpy(block.content[j].name, "\\\\\\\\\\\\\\\\\\\\\\\\", 12);
            }

            inodo.block[i] = super.first_block;
            fseek(file, super.bm_block_start + inodo.block[i], SEEK_SET);
            fwrite("\1", 1, 1, file);
            super.first_block = get_first_block(file, super);
            super.free_blocks_count -= 1;

            num_block = i;
            num_content = 0;
            found = true;
        }

        if (found)
            break;
    }

    if (!found)
        throw Exception("no space left");

    strncpy(block.content[num_content].name, new_file.c_str(), 12);
    block.content[num_content].inode = super.first_inode;
    fseek(file, super.block_start + inodo.block[num_block] * sizeof(folder_block), SEEK_SET);
    fwrite(&block, sizeof(folder_block), 1, file);

    inode inodo_f;
    strncpy(inodo_f.ctime, date, 16);
    strncpy(inodo_f.mtime, date, 16);
    inodo_f.type = '\1';
    inodo_f.permissions = inodo.permissions;
    inodo_f.uid = user_num;
    inodo_f.gid = group_num;

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

    for (int i = 0; i < 12; i++)
    {
        if (i * 64 > content.length())
            break;

        temp = content.substr(i * 64, 64);

        file_block block;
        inodo_f.block[i] = super.first_block;
        strncpy(block.content, temp.c_str(), 64);

        fseek(file, super.bm_block_start + inodo_f.block[i], SEEK_SET);
        fwrite("\1", 1, 1, file);
        fseek(file, super.block_start + inodo_f.block[i] * sizeof(file_block), SEEK_SET);
        fwrite(&block, sizeof(file_block), 1, file);

        super.first_block = get_first_block(file, super);
        super.free_blocks_count -= 1;
    }

    fseek(file, super.bm_inode_start + super.first_inode, SEEK_SET);
    fwrite("\1", 1, 1, file);
    fseek(file, super.inode_start + super.first_inode * sizeof(inode), SEEK_SET);
    fwrite(&inodo_f, sizeof(inode), 1, file);
    super.first_inode = get_first_inode(file, super);
    super.free_inodes_count -= 1;

    strncpy(inodo.mtime, date, 16);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

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

bool is_deleteable(FILE *file, superblock super, inode inodo)
{
    if (!has_permission(inodo, "w"))
        return false;

    bool value = true;

    if (inodo.type == '\0')
        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            folder_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
            {
                if (string(block.content[j].name) == "." || string(block.content[j].name) == ".." || block.content[j].inode == -1)
                    continue;

                inode inodo_temp;
                fseek(file, super.inode_start + block.content[j].inode * sizeof(inode), SEEK_SET);
                fread(&inodo_temp, sizeof(inode), 1, file);

                value = value && is_deleteable(file, super, inodo_temp);
            }
        }

    return value;
}

int delete_inode(FILE *file, superblock super, inode inodo)
{
    if (inodo.type == '\0')
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
                if (string(block.content[j].name) == "." || string(block.content[j].name) == ".." || block.content[j].inode == -1)
                    continue;

                inode inodo_temp;
                fseek(file, super.inode_start + block.content[j].inode * sizeof(inode), SEEK_SET);
                fread(&inodo_temp, sizeof(inode), 1, file);

                delete_inode(file, super, inodo_temp);

                fseek(file, super.bm_inode_start + block.content[j].inode, SEEK_SET);
                fwrite("\0", 1, 1, file);
                fseek(file, super.inode_start + block.content[j].inode * sizeof(inode), SEEK_SET);
                fwrite("\0", sizeof(inode), 1, file);
            }

            fseek(file, super.bm_block_start + inodo.block[i], SEEK_SET);
            fwrite("\0", 1, 1, file);
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fwrite("\0", sizeof(folder_block), 1, file);
        }
    }

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

    file = fopen(disk_path, "r+b");

    int num_block = 0, num_nodo = 0, num_content = 0;
    inode inodo;

    for (string dir : words)
    {
        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
        strncpy(inodo.atime, date, 16);

        if (inodo.type == '\1')
            throw Exception("a folder path is required");

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
                    num_block = inodo.block[i];
                    num_content = j;
                    found = true;
                    break;
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
    }

    if (!has_permission(inodo, "w"))
    {
        string msg = "write access denied";
        throw Exception(msg.c_str());
    }

    inode inodo_temp;
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fread(&inodo_temp, sizeof(inode), 1, file);

    if (!is_deleteable(file, super, inodo_temp))
        throw Exception("could not delete one of more contents");

    delete_inode(file, super, inodo_temp);

    folder_block block;
    fseek(file, super.block_start + num_block * sizeof(folder_block), SEEK_SET);
    fread(&block, sizeof(folder_block), 1, file);
    block.content[num_content].inode = -1;
    strncpy(block.content[num_content].name, "\\\\\\\\\\\\\\\\\\\\\\\\", 12);
    fseek(file, super.block_start + num_block * sizeof(folder_block), SEEK_SET);
    fwrite(&block, sizeof(folder_block), 1, file);

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

    return 0;
}

int ren(string path, string name)
{

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

    string new_dir = words.at(words.size() - 1);
    words.pop_back();

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    file = fopen(disk_path, "r+b");

    int num_nodo = 0;
    string folder = "/";

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
                    break;
                }
            }

            if (found)
                break;
        }

        if (!found)
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

                bool space_found = false;
                for (int j = 0; j < 12; j++)
                {
                    folder_block bloque_temp;

                    if (inodo.block[j] == -1)
                    {
                        inodo.block[j] = super.first_block;
                        fseek(file, super.bm_block_start + super.first_block, SEEK_SET);
                        fwrite("\1", 1, 1, file);
                        super.free_blocks_count -= 1;
                        super.first_block = get_first_block(file, super);

                        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
                        fwrite(&inodo, sizeof(inode), 1, file);
                    }
                    else
                    {
                        fseek(file, super.block_start + inodo.block[j] * sizeof(folder_block), SEEK_SET);
                        fread(&bloque_temp, sizeof(folder_block), 1, file);
                    }

                    for (int k = 0; k < 4; k++)
                    {
                        if (bloque_temp.content[k].inode == -1)
                        {
                            bloque_temp.content[k].inode = super.first_inode;
                            strncpy(bloque_temp.content[k].name, dir.c_str(), 12);
                            fseek(file, super.block_start + inodo.block[j] * sizeof(folder_block), SEEK_SET);
                            fwrite(&bloque_temp, sizeof(folder_block), 1, file);

                            space_found = true;
                            break;
                        }
                    }

                    if (space_found)
                        break;
                }

                // nuevo inodo carpeta, bloque del inodo carpeta
                inode new_inodo;
                new_inodo.block[0] = super.first_block;
                strncpy(new_inodo.ctime, date, 16);

                folder_block bloque;
                strcpy(bloque.content[0].name, ".");
                bloque.content[0].inode = super.first_inode;
                strcpy(bloque.content[1].name, "..");
                bloque.content[1].inode = num_nodo;

                fseek(file, super.bm_inode_start + super.first_inode, SEEK_SET);
                fwrite("\1", 1, 1, file);
                fseek(file, super.inode_start + super.first_inode * sizeof(inode), SEEK_SET);
                fwrite(&new_inodo, sizeof(inode), 1, file);

                fseek(file, super.bm_block_start + super.first_block, SEEK_SET);
                fwrite("\1", 1, 1, file);
                fseek(file, super.block_start + super.first_block * sizeof(folder_block), SEEK_SET);
                fwrite(&bloque, sizeof(folder_block), 1, file);

                num_nodo = super.first_inode;

                super.free_inodes_count -= 1;
                super.first_inode = get_first_inode(file, super);
                super.free_blocks_count -= 1;
                super.first_block = get_first_block(file, super);
            }
        }

        folder = dir;

        strncpy(inodo.atime, date, 16);
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

    bool found = false, replace = false;
    int num_block, num_content;
    folder_block block;

    // buscando espacio en la ultima carpeta
    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] != -1)
        {
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
            {
                if (block.content[j].name == new_dir)
                    throw Exception("already existent folder");
                else if (block.content[j].inode != -1)
                    continue;

                num_block = i;
                num_content = j;
                found = true;
                break;
            }
        }
        else
        {
            for (int j = 0; j < 4; j++)
            {
                block.content[j].inode = -1;
                strncpy(block.content[j].name, "\\\\\\\\\\\\\\\\\\\\\\\\", 12);
            }

            inodo.block[i] = super.first_block;
            fseek(file, super.bm_block_start + inodo.block[i], SEEK_SET);
            fwrite("\1", 1, 1, file);
            super.first_block = get_first_block(file, super);
            super.free_blocks_count -= 1;

            num_block = i;
            num_content = 0;
            found = true;
        }

        if (found)
            break;
    }

    if (!found)
        throw Exception("no space left");

    strncpy(block.content[num_content].name, new_dir.c_str(), 12);
    block.content[num_content].inode = super.first_inode;
    fseek(file, super.block_start + inodo.block[num_block] * sizeof(folder_block), SEEK_SET);
    fwrite(&block, sizeof(folder_block), 1, file);

    inode inodo_f;
    strncpy(inodo_f.ctime, date, 16);
    strncpy(inodo_f.mtime, date, 16);
    inodo_f.permissions = inodo.permissions;
    inodo_f.uid = user_num;
    inodo_f.gid = group_num;
    inodo_f.block[0] = super.first_block;

    folder_block bloque;
    strcpy(bloque.content[0].name, ".");
    bloque.content[0].inode = super.first_inode;
    strcpy(bloque.content[1].name, "..");
    bloque.content[1].inode = num_nodo;
    fseek(file, super.bm_block_start + super.first_block, SEEK_SET);
    fwrite("\1", 1, 1, file);
    fseek(file, super.block_start + super.first_block * sizeof(folder_block), SEEK_SET);
    fwrite(&bloque, sizeof(block), 1, file);
    super.first_block = get_first_block(file, super);
    super.free_blocks_count -= 1;

    create_journal(file, &super, date, "mkdir", path, "", 0, '\0');

    fseek(file, super.bm_inode_start + super.first_inode, SEEK_SET);
    fwrite("\1", 1, 1, file);
    fseek(file, super.inode_start + super.first_inode * sizeof(inode), SEEK_SET);
    fwrite(&inodo_f, sizeof(inode), 1, file);
    super.first_inode = get_first_inode(file, super);
    super.free_inodes_count -= 1;

    strncpy(inodo.mtime, date, 16);
    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);

    strncpy(super.umtime, date, 16);
    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fclose(file);

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

    string content = "";

    for (int i = 0; i < 12; i++)
    {
        if (inodo_users.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo_users.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content).substr(0, sizeof(file_block));
    }

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
