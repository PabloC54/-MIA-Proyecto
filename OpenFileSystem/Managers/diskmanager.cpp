#include <iostream>
#include <fstream>
#include <cstdlib>
#include "time.h"
#include <string>
#include "diskmanager.h"
#include "../Util/util.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"

using namespace std;
disk mounted[26];

int mkdisk(string size, string f, string u, string path)
{
    if (f.empty())
        f = "ff";
    if (u.empty())
        u = "m";

    if (f != "bf" && f != "ff" && f != "wf")
        throw Exception("-f parameter non-valid. Valid: bf (best fit), ff (first fit), wf (worst fit)");

    int units;

    if (u == "k")
        units = 1024;
    else if (u == "m")
        units = 1024 * 1024;
    else
        throw Exception("-u parameter non-valid. Valid: k (kilobytes), m (megabytes)");

    int size_int = stoi(size);

    if (u == "k")
        size_int = size_int * 1024;
    else
        size_int = size_int * 1024 * 1024;

    if (size_int < 0)
        throw Exception("disk size was too big");

    FILE *file = fopen(path.c_str(), "r");
    if (file != NULL)
    {
        fclose(file);
        throw Exception("disk already created");
    }

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[16];
    strftime(date, 16, "%d/%m/%Y %H:%M", tm);

    MBR mbr;
    mbr.size = size_int;
    strcpy(mbr.date, date);
    mbr.signature = rand() % 100;
    strcpy(mbr.fit, f.c_str());

    string command_make = "mkdir -p \"" + path + "\"";
    string command_remove = "rmdir \"" + path + "\"";

    if (system(command_make.c_str()) != 0)
        throw Exception("could not create path");

    system(command_remove.c_str());

    file = fopen(path.c_str(), "wb");

    fwrite("\0", 1, 1, file);
    fseek(file, size_int - 1, SEEK_SET);
    fwrite("\0", 1, 1, file);
    fseek(file, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MBR), 1, file);

    fclose(file);

    return 0;
}

int rmdisk(string path)
{
    FILE *file = fopen(path.c_str(), "r");
    if (file == NULL)
        throw Exception("specified disk does not exist");

    string command_remove = "rm \"" + path + "\"";

    if (system(command_remove.c_str()) != 0)
        throw Exception("could not remove disk");

    fclose(file);
    return 0;
}

int fdisk(string size, string u, string path, string type, string f, string _delete, string name, string add)
{
    if (u.empty())
        u = "k";
    if (f.empty())
        f = "wf";
    if (type.empty())
        type = "p";

    if (!_delete.empty() && !add.empty())
        throw Exception("-add, -delete parameters are incompatible");

    if (f != "bf" && f != "ff" && f != "wf")
        throw Exception("-f parameter non-valid. Valid: bf (best fit), ff (first fit), wf (worst fit)");

    if (type != "p" && type != "e" && type != "l")
        throw Exception("-type parameter non-valid. Valid: p (primary), e (extended), l (logical)");

    FILE *file = fopen(path.c_str(), "rb");

    if (file == NULL)
        throw Exception("disk does not exist");

    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    if (add.empty() && _delete.empty())
    { // CREATE MODE
        if (size.empty())
            throw Exception("-size parameter is needed when creating a partition");

        for (Partition par : mbr.partitions)
            if (par.name == name)
                throw Exception("partition name in use");

        int units;

        if (u == "b")
            units = 1;
        else if (u == "k")
            units = 1024;
        else if (u == "m")
            units = 1024 * 1024;
        else
            throw Exception("-u parameter non-valid. Valid: b (bytes), k (kilobytes), m (megabytes)");

        int size_int = stoi(size) * units;

        if (size_int < 0)
            throw Exception("partition size was too big");

        if (type == "p")
        {
            Partition *partition = getNewPatition(&mbr, f);
            if (partition->status == '\\')
                throw Exception("disk already has 4 primary partitions");

            strcpy(partition->name, name.c_str());
            partition->type = type[0];
            partition->size = size_int;
        }
        else if (type == "e")
        {
            Partition *partition = getNewPatition(&mbr, f);
            if (partition->status == '\\')
                throw Exception("disk already has 4 primary partitions");

            for (Partition par : mbr.partitions)
                if (par.type == 'e')
                    throw Exception("disk already has an extended partition");

            strcpy(partition->name, name.c_str());
            partition->status = '\0';
            partition->type = type[0];
            partition->size = size_int;

            EBR ebr;
            ebr.name[0] = '\\';
            ebr.size = -1;
            ebr.start = -1;
            ebr.next = -1;
            strcpy(ebr.fit, f.c_str());

            fseek(file, partition->start, SEEK_SET);
            fwrite(&ebr, sizeof(ebr), 1, file);
        }
        else
        { // PARTICIÓN LÓGICA
        }
    }
    else if (!add.empty())
    { // RESIZE MODE
        Partition *partition = getPartition(&mbr, name);

        if (partition->status == '\\')
            throw Exception("specified partition does not exist");

        int units;

        if (u == "b")
            units = 1;
        else if (u == "k")
            units = 1024;
        else if (u == "m")
            units = 1024 * 1024;
        else
            throw Exception("-u parameter non-valid. Valid: b (bytes), k (kilobytes), m (megabytes)");

        int size_int = stoi(add) * units;

        if (size_int < 0)
            throw Exception("disk size was too big");

        if (!isResizable(mbr, name, size_int))
        {
            string msg = "partition cannot be resized by " + add + u;
            throw Exception(msg.c_str());
        }

        partition->size = partition->size + size_int;
    }
    else
    { // DELETE MODE
        if (_delete != "fast" && _delete != "full")
            throw Exception("-delete parameter non-valid. Valid: fast, full");

        int units;

        if (u == "b")
            units = 1;
        else if (u == "k")
            units = 1024;
        else if (u == "m")
            units = 1024 * 1024;
        else
            throw Exception("-u parameter non-valid. Valid: b (bytes), k (kilobytes), m (megabytes)");

        int size_int = stoi(_delete) * units;

        if (size_int < 0)
            throw Exception("disk size was too big");

        Partition *partition = getPartition(&mbr, name);

        if (partition->status != '\\')
        {
            if (partition->type == 'p')
            { // PRIMARIA

                Partition new_partition;
                partition = &new_partition;

                if (_delete == "full")
                {
                    // Rellenar con \0
                }
            }
            else if (partition->type == 'e')
            { // EXTENDIDA

                Partition new_partition;
                partition = &new_partition;

                // eliminar particiones logicas si tiene

                if (_delete == "full")
                {
                    // Rellenar con \0
                }
            }
            else
            {
                throw Exception("specified partition does not exist");
            }
        }
        else
        { // LÓGICA

            for (Partition par : mbr.partitions)
            {
                if (par.type == 'e')
                {
                    partition = &par;
                }
            }

            if (partition->status = '\\')
                throw Exception("specified partition does not exist");

            fseek(file, partition->start, SEEK_SET);

            EBR ebr;
            fread(&ebr, sizeof(EBR), 1, file);

            while (ebr.next != -1)
            {
                if (ebr.name == name.c_str())
                {
                    // eliminar

                    if (_delete == "full")
                    {
                        // Rellenar con \0
                    }

                    break;
                }
                else
                {
                    fseek(file, ebr.next, SEEK_SET);
                    fread(&ebr, sizeof(EBR), 1, file);
                }
            }

            if (ebr.next == -1)
                throw Exception("specified partition does not exist");
        }
    }
    fclose(file);
    file = fopen(path.c_str(), "r+b");
    fseek(file, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MBR), 1, file);
    fclose(file);

    return 0;
}

int mount(string path, string name)
{

    // if (path.empty() && name.empty())
    // {

    //     return 0;
    // }

    FILE *file = fopen(path.c_str(), "rb");
    if (file == NULL)
        throw Exception("disk does not exist");

    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(&mbr, name);

    if (partition->status == '\\')
        throw Exception("partition does not exist");

    disk new_disk;
    disk *dk = &new_disk;

    for (int i = 0; i < 26; i++)
    {
        if (string(mounted[i].path).substr(0, path.length()) == path)
        {
            dk = &mounted[i];
            break;
        }
    }

    if (dk->status != 1)
    {
        if (mounted[25].status == 1)
            throw Exception("max disk mounted count reached");

        for (int i = 0; i < 26; i++)
        {
            if (mounted[i].status == -1)
            {
                dk = &mounted[i];
                break;
            }
        }

        dk->id = getDiskId();
        dk->status = 1;
        strcpy(dk->path, path.c_str());
    }

    for (int i = 0; i < 99; i++)
        if (string(dk->partitions[i].name).substr(0, name.length()) == name)
            throw Exception("partition is already mounted");

    MountStructs::partition new_partition;
    MountStructs::partition *par = &new_partition;

    for (int i = 0; i < 99; i++)
        if (dk->partitions[i].status == -1)
        {
            dk->partitions[i].status = 1;
            par = &dk->partitions[i];
            break;
        }

    if (par->status == -1)
        throw Exception("max partition mounted count reached for this disk");

    par->id = getPartitionId(*dk);
    strcpy(par->name, name.c_str());

    cout << "mounted with id '" << endl;

    return 0;
}

int unmount(string id)
{
    if (id.length() < 4 || id.length() > 5)
        throw Exception("non-valid id");

    if (id.substr(0, 2) != "98")
        throw Exception("non-valid id (must start with '98')");

    int disk_i = -1, par_i = -1;

    if (id.length() == 4)
    {
        char disk_id = id.substr(3, 4)[0];
        int par_id = stoi(id.substr(2, 3));

        for (int i = 0; i < 26; i++)
            if (mounted[i].id == disk_id)
                for (int j = 0; j < 99; j++)
                    if (mounted[i].partitions[j].id == par_id)
                    {
                        disk_i = i;
                        par_i = j;
                    }
    }
    else
    {
        char disk_id = id.substr(4, 5)[0];
        int par_id = stoi(id.substr(2, 4));

        for (int i = 0; i < 26; i++)
            if (mounted[i].id == disk_id)
                for (int j = 0; j < 99; j++)
                    if (mounted[i].partitions[j].id == par_id)
                    {
                        disk_i = i;
                        par_i = j;
                    }
    }

    if (disk_i == -1 || par_i == -1)
        throw Exception("partition not mounted");

    mounted[disk_i].partitions[par_i].id = -1;
    mounted[disk_i].partitions[par_i].status = -1;

    for (MountStructs::partition par : mounted[disk_i].partitions)
        if (par.id != -1)
            return 0;

    mounted[disk_i].status = -1;
    return 0;
}

int mkfs(string id, string type, string fs)
{
    if (type.empty())
        type = "full";

    if (fs.empty())
        fs = "2fs";

    if (type != "fast" && type != "full")
        throw Exception("-type parameter non-valid. Valid: fast, full");

    if (fs != "2fs" && fs != "3fs")
        throw Exception("-fs parameter non-valid. Valid: 2fs (EXT2), 3fs (EXT3)");

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

    if (partition->status == '1')
    {
        cout << endl
             << "already formated partition. reformat? (y/n)" << endl;
        string rep;
        cin >> rep;

        if (rep == "n" || rep == "N")
        {
            cout << "reformat cancelled" << endl;
            return 0;
        }
    }

    int num_structs;
    superblock super;

    super.inode_size = sizeof(inode);
    super.block_size = sizeof(file_block);

    if (fs == "2fs")
    {
        super.filesystem_type = 2;

        num_structs = (partition->size - sizeof(superblock)) /
                      (4 + sizeof(inode) + 3 * sizeof(file_block));

        super.inodes_count = num_structs;
        super.free_inodes_count = num_structs;
        super.blocks_count = 3 * num_structs;
        super.free_blocks_count = 3 * num_structs;

        super.bm_inode_start = partition->start + sizeof(superblock);
        super.bm_block_start = super.bm_inode_start + super.inodes_count;
        super.inode_start = super.bm_block_start + super.blocks_count;
        super.block_start = super.inode_start + super.inode_size * super.inodes_count;
    }
    else
    {
        super.filesystem_type = 3;

        num_structs = (partition->size - sizeof(superblock)) /
                      (4 + sizeof(journal) + sizeof(inode) + 3 * sizeof(file_block));

        super.journal_size = sizeof(journal);

        super.journal_count = num_structs;
        super.free_journal_count = num_structs;
        super.inodes_count = num_structs;
        super.free_inodes_count = num_structs;
        super.blocks_count = 3 * num_structs;
        super.free_blocks_count = 3 * num_structs;

        super.journal_start = partition->start + sizeof(superblock);
        super.bm_inode_start = super.journal_start + super.journal_size * super.journal_count;
        super.bm_block_start = super.bm_inode_start + super.inodes_count;
        super.inode_start = super.bm_block_start + super.blocks_count;
        super.block_start = super.inode_start + super.inode_size * super.inodes_count;

        super.first_journal = 0;
    }

    partition->status = '1';

    // creando el primer inodo y bloques para users.txt

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[16];
    strftime(date, 16, "%d/%m/%Y %H:%M", tm);

    inode inodo;
    strcpy(inodo.ctime, date);

    // crear los primeros journal

    folder_block bloque;
    strcpy(bloque.content[0].name, ".");
    bloque.content[0].inode = 0;
    strcpy(bloque.content[1].name, "..");
    bloque.content[1].inode = 0;
    strcpy(bloque.content[2].name, "users.txt");
    bloque.content[2].inode = 1;
    inodo.block[0] = 0;

    inode inodo2;
    strcpy(inodo2.ctime, date);
    inodo2.type = '\1';
    inodo2.block[0] = 1;

    file_block bloque2;
    strcpy(bloque2.content, "1,G,root\n1,U,root,root,123\n");

    super.free_inodes_count -= 2;
    super.first_inode += 2;
    super.free_blocks_count -= 2;
    super.first_block += 2;

    // escribiendo todo
    file = fopen(path, "r+b");

    fseek(file, super.bm_inode_start, SEEK_SET);
    fwrite("\1\1", 2, 1, file);
    fseek(file, super.bm_block_start, SEEK_SET);
    fwrite("\1\1", 2, 1, file);

    fseek(file, super.inode_start, SEEK_SET);
    fwrite(&inodo, sizeof(inode), 1, file);
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fwrite(&inodo2, sizeof(inode), 1, file);

    fseek(file, super.block_start, SEEK_SET);
    fwrite(&bloque, sizeof(file_block), 1, file);
    fseek(file, super.block_start + sizeof(file_block), SEEK_SET);
    fwrite(&bloque2, sizeof(file_block), 1, file);

    fseek(file, partition->start, SEEK_SET);
    fwrite(&super, sizeof(superblock), 1, file);

    fseek(file, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MBR), 1, file);

    fclose(file);
    return 0;
}
