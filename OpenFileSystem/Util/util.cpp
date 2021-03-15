#include <iostream>
#include <regex>
#include <boost/algorithm/string.hpp>

#include "../Managers/usermanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"

using namespace std;

string unquote(string word)
{
    if (boost::algorithm::ends_with(word, "\"") || boost::algorithm::ends_with(word, "'"))
    {
        word = word.substr(0, word.length() - 1);
    }
    if (boost::algorithm::starts_with(word, "\"") || boost::algorithm::starts_with(word, "'"))
    {
        word = word.substr(1, word.length() - 1);
    }

    return word;
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

string read_inode_content(FILE *file, superblock super, inode inodo)
{
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

    return content;
}

int write_inode_content(FILE *file, superblock *super, inode *inodo, string content)
{
    string temp;
    bool b_break = false;

    for (int i = 0; i < 12; i++)
    {
        if (((i + 1) * 64) >= content.length())
        {
            temp = content.substr(i * 64, content.length() - i * 64);
            b_break = true;
        }
        else
        {
            temp = content.substr(i * 64, 64);
        }

        file_block block;
        strcpy(block.content, temp.c_str());

        if (inodo->block[i] == -1)
        {
            inodo->block[i] = super->first_block;
            fseek(file, super->bm_block_start + super->first_block, SEEK_SET);
            fwrite("\1", 1, 1, file);

            super->free_blocks_count -= 1;
            super->first_block = get_first_block(file, *super);
        }

        fseek(file, super->block_start + inodo->block[i] * sizeof(file_block), SEEK_SET);
        fwrite(&block, sizeof(file_block), 1, file);

        if (b_break)
            break;
    }

    return 0;
}

int new_folder(FILE *file, superblock *super, inode *inodo, int old_num, string name, const char *date)
{
    int num_inode = super->first_inode;
    bool space_found = false;

    for (int i = 0; i < 12; i++)
    {
        folder_block bloque_temp;

        if (inodo->block[i] == -1)
        {
            inodo->block[i] = super->first_block;
            fseek(file, super->bm_block_start + super->first_block, SEEK_SET);
            fwrite("\1", 1, 1, file);
            super->free_blocks_count -= 1;
            super->first_block = get_first_block(file, *super);

            fseek(file, super->inode_start + old_num * sizeof(inode), SEEK_SET);
            fwrite(inodo, sizeof(inode), 1, file);
        }
        else
        {
            fseek(file, super->block_start + inodo->block[i] * sizeof(folder_block), SEEK_SET);
            fread(&bloque_temp, sizeof(folder_block), 1, file);
        }

        for (int j = 0; j < 4; j++)
        {
            if (bloque_temp.content[j].inode == -1)
            {
                bloque_temp.content[j].inode = num_inode;
                strncpy(bloque_temp.content[j].name, name.c_str(), 12);
                fseek(file, super->block_start + inodo->block[i] * sizeof(folder_block), SEEK_SET);
                fwrite(&bloque_temp, sizeof(folder_block), 1, file);

                space_found = true;
                break;
            }
        }

        if (space_found)
            break;
    }

    inode new_inodo;
    strncpy(new_inodo.ctime, date, 16);
    new_inodo.block[0] = super->first_block;

    folder_block block;
    strcpy(block.content[0].name, ".");
    block.content[0].inode = num_inode;
    strcpy(block.content[1].name, "..");
    block.content[1].inode = old_num;

    fseek(file, super->bm_inode_start + num_inode, SEEK_SET);
    fwrite("\1", 1, 1, file);
    fseek(file, super->inode_start + num_inode * sizeof(inode), SEEK_SET);
    fwrite(&new_inodo, sizeof(inode), 1, file);

    fseek(file, super->bm_block_start + super->first_block, SEEK_SET);
    fwrite("\1", 1, 1, file);
    fseek(file, super->block_start + super->first_block * sizeof(folder_block), SEEK_SET);
    fwrite(&block, sizeof(folder_block), 1, file);

    super->free_inodes_count -= 1;
    super->first_inode = get_first_inode(file, *super);
    super->free_blocks_count -= 1;
    super->first_block = get_first_block(file, *super);

    return num_inode;
}

int new_file(FILE *file, superblock *super, inode *inodo, int old_num, string name, const char *date)
{
    int num_inode = super->first_inode;
    bool space_found = false;

    for (int i = 0; i < 12; i++)
    {
        folder_block bloque_temp;

        if (inodo->block[i] == -1)
        {
            inodo->block[i] = super->first_block;
            fseek(file, super->bm_block_start + super->first_block, SEEK_SET);
            fwrite("\1", 1, 1, file);
            super->free_blocks_count -= 1;
            super->first_block = get_first_block(file, *super);

            fseek(file, super->inode_start + old_num * sizeof(inode), SEEK_SET);
            fwrite(inodo, sizeof(inode), 1, file);
        }
        else
        {
            fseek(file, super->block_start + inodo->block[i] * sizeof(folder_block), SEEK_SET);
            fread(&bloque_temp, sizeof(folder_block), 1, file);
        }

        for (int j = 0; j < 4; j++)
        {
            if (bloque_temp.content[j].inode == -1)
            {
                bloque_temp.content[j].inode = num_inode;
                strncpy(bloque_temp.content[j].name, name.c_str(), 12);
                fseek(file, super->block_start + inodo->block[i] * sizeof(folder_block), SEEK_SET);
                fwrite(&bloque_temp, sizeof(folder_block), 1, file);

                space_found = true;
                break;
            }
        }

        if (space_found)
            break;
    }

    inode new_inodo;
    strncpy(new_inodo.ctime, date, 16);
    new_inodo.block[0] = super->first_block;

    fseek(file, super->bm_inode_start + num_inode, SEEK_SET);
    fwrite("\1", 1, 1, file);
    fseek(file, super->inode_start + num_inode * sizeof(inode), SEEK_SET);
    fwrite(&new_inodo, sizeof(inode), 1, file);

    super->free_inodes_count -= 1;
    super->first_inode = get_first_inode(file, *super);

    return num_inode;
}

bool exists_inode(FILE *file, superblock *super, inode *inodo, string name)
{
    for (int i = 0; i < 12; i++)
    {
        if (inodo->block[i] == -1)
            continue;

        folder_block block;
        fseek(file, super->block_start + inodo->block[i] * sizeof(folder_block), SEEK_SET);
        fread(&block, sizeof(folder_block), 1, file);

        for (int j = 0; j < 4; j++)
            if (block.content[j].name == name)
                return true;
    }

    pointer_block block_one, block_two, block_three;
    folder_block block;

    // indirecto simple
    if (inodo->block[12] != -1)
    {
        fseek(file, super->block_start + inodo->block[12] * sizeof(pointer_block), SEEK_SET);
        fread(&block_one, sizeof(pointer_block), 1, file);

        for (int i = 0; i < 16; i++)
        {
            if (block_one.pointers[i] == -1)
                continue;

            fseek(file, super->block_start + block_one.pointers[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
                if (block.content[j].name == name)
                    return true;
        }
    }

    // indirecto doble
    if (inodo->block[13] != -1)
    {
        fseek(file, super->block_start + inodo->block[13] * sizeof(pointer_block), SEEK_SET);
        fread(&block_one, sizeof(pointer_block), 1, file);

        for (int i = 0; i < 16; i++)
        {
            if (block_one.pointers[i] == -1)
                continue;

            fseek(file, super->block_start + block_one.pointers[i] * sizeof(pointer_block), SEEK_SET);
            fread(&block_two, sizeof(pointer_block), 1, file);

            for (int j = 0; j < 16; j++)
            {
                if (block_two.pointers[j] == -1)
                    continue;

                fseek(file, super->block_start + block_two.pointers[i] * sizeof(folder_block), SEEK_SET);
                fread(&block, sizeof(folder_block), 1, file);

                for (int k = 0; k < 4; k++)
                    if (block.content[k].name == name)
                        return true;
            }
        }
    }

    // indirecto triple
    if (inodo->block[14] != -1)
    {
        fseek(file, super->block_start + inodo->block[14] * sizeof(pointer_block), SEEK_SET);
        fread(&block_one, sizeof(pointer_block), 1, file);

        for (int i = 0; i < 16; i++)
        {
            if (block_one.pointers[i] == -1)
                continue;

            fseek(file, super->block_start + block_one.pointers[i] * sizeof(pointer_block), SEEK_SET);
            fread(&block_two, sizeof(pointer_block), 1, file);

            for (int j = 0; j < 16; j++)
            {
                if (block_two.pointers[j] == -1)
                    continue;

                fseek(file, super->block_start + block_two.pointers[j] * sizeof(pointer_block), SEEK_SET);
                fread(&block_three, sizeof(pointer_block), 1, file);

                for (int k = 0; k < 16; k++)
                {
                    if (block_three.pointers[k] == -1)
                        continue;

                    fseek(file, super->block_start + block_three.pointers[k] * sizeof(folder_block), SEEK_SET);
                    fread(&block, sizeof(folder_block), 1, file);

                    for (int l = 0; l < 4; l++)
                        if (block.content[l].name == name)
                            return true;
                }
            }
        }
    }

    return false;
}

int get_inode(FILE *file, superblock *super, inode *inodo, string name)
{
    int num = 0;

    for (int i = 0; i < 12; i++)
    {
        if (inodo->block[i] == -1)
            continue;

        folder_block block;
        fseek(file, super->block_start + inodo->block[i] * sizeof(folder_block), SEEK_SET);
        fread(&block, sizeof(folder_block), 1, file);

        for (int j = 0; j < 4; j++)
            if (block.content[j].name == name)
                num = block.content[j].inode;
    }

    pointer_block block_one, block_two, block_three;
    folder_block block;

    // indirecto simple
    if (inodo->block[12] != -1)
    {
        fseek(file, super->block_start + inodo->block[12] * sizeof(pointer_block), SEEK_SET);
        fread(&block_one, sizeof(pointer_block), 1, file);

        for (int i = 0; i < 16; i++)
        {
            if (block_one.pointers[i] == -1)
                continue;

            fseek(file, super->block_start + block_one.pointers[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
                if (block.content[j].name == name)
                    num = block.content[j].inode;
        }
    }

    // indirecto doble
    if (inodo->block[13] != -1)
    {
        fseek(file, super->block_start + inodo->block[13] * sizeof(pointer_block), SEEK_SET);
        fread(&block_one, sizeof(pointer_block), 1, file);

        for (int i = 0; i < 16; i++)
        {
            if (block_one.pointers[i] == -1)
                continue;

            fseek(file, super->block_start + block_one.pointers[i] * sizeof(pointer_block), SEEK_SET);
            fread(&block_two, sizeof(pointer_block), 1, file);

            for (int j = 0; j < 16; j++)
            {
                if (block_two.pointers[j] == -1)
                    continue;

                fseek(file, super->block_start + block_two.pointers[i] * sizeof(folder_block), SEEK_SET);
                fread(&block, sizeof(folder_block), 1, file);

                for (int k = 0; k < 4; k++)
                    if (block.content[k].name == name)
                        num = block.content[k].inode;
            }
        }
    }

    // indirecto triple
    if (inodo->block[14] != -1)
    {
        fseek(file, super->block_start + inodo->block[14] * sizeof(pointer_block), SEEK_SET);
        fread(&block_one, sizeof(pointer_block), 1, file);

        for (int i = 0; i < 16; i++)
        {
            if (block_one.pointers[i] == -1)
                continue;

            fseek(file, super->block_start + block_one.pointers[i] * sizeof(pointer_block), SEEK_SET);
            fread(&block_two, sizeof(pointer_block), 1, file);

            for (int j = 0; j < 16; j++)
            {
                if (block_two.pointers[j] == -1)
                    continue;

                fseek(file, super->block_start + block_two.pointers[j] * sizeof(pointer_block), SEEK_SET);
                fread(&block_three, sizeof(pointer_block), 1, file);

                for (int k = 0; k < 16; k++)
                {
                    if (block_three.pointers[k] == -1)
                        continue;

                    fseek(file, super->block_start + block_three.pointers[k] * sizeof(folder_block), SEEK_SET);
                    fread(&block, sizeof(folder_block), 1, file);

                    for (int l = 0; l < 4; l++)
                        if (block.content[l].name == name)
                            num = block.content[l].inode;
                }
            }
        }
    }

    return num;
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

    if (string(access) == "r")
        return r == 1;
    if (string(access) == "w")
        return w == 1;
    if (string(access) == "x")
        return x == 1;

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

int copy_inode(FILE *file, superblock *super, inode inodo)
{
    int origin_inode, origin_block;

    if (inodo.type == '\0')
    {
        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            origin_block = inodo.block[i];

            folder_block block;
            fseek(file, super->block_size + origin_block * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            //adaptacion bloque

            for (int j = 0; j < 4; j++)
            {
                if (string(block.content[j].name) == "." || string(block.content[j].name) == ".." || block.content[j].inode == -1)
                    continue;

                inode inodo_temp;
                fseek(file, super->inode_start + block.content[j].inode * sizeof(inode), SEEK_SET);
                fread(&inodo_temp, sizeof(inode), 1, file);

                // adaptacion inodo

                //copia inodo
                block.content[j].inode = super->first_inode;

                fseek(file, super->bm_inode_start + block.content[j].inode, SEEK_SET);
                fwrite("\0", 1, 1, file);
                fseek(file, super->inode_start + block.content[j].inode * sizeof(inode), SEEK_SET);
                fwrite("\0", sizeof(inode), 1, file);

                copy_inode(file, super, inodo_temp);
            }

            // copia bloque
            inodo.block[i] = super->first_block;

            fseek(file, super->bm_block_start + super->first_block, SEEK_SET);
            fwrite("\1", 1, 1, file);
            fseek(file, super->block_start + super->first_block * sizeof(folder_block), SEEK_SET);
            fwrite(&block, sizeof(folder_block), 1, file);
        }
    }

    return 0;
}

string find_inode(FILE *file, superblock super, inode inodo, string name, int deep, regex f)
{
    string s = "", pre = "";
    for (int i = 0; i < deep; i++)
        pre += "  ";

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

                if (inodo_temp.type == '\1')
                {
                    if (regex_search(block.content[j].name, f))
                        s += pre + "|_" + string(block.content[j].name) + "\n";
                }
                else
                {
                    string temp = find_inode(file, super, inodo_temp, string(block.content[j].name), deep + 1, f);
                    if (!temp.empty())
                    {
                        s += pre + "|_" + string(block.content[j].name) + "\n" + temp;
                    }
                }
            }
        }
    }

    return s;
}
