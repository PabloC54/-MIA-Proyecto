#include <iostream>
#include <fstream>
#include <cstdlib>
#include "time.h"
#include "diskmanager.h"
#include "../Util/util.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"

vector<disk> mounted;

using namespace std;

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

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char date[16];
    strftime(date, 16, "%d/%m/%Y %H:%M", tm);

    MBR mbr;
    mbr.size = size_int;
    strcpy(mbr.date, date);
    mbr.signature = rand() % 100;
    mbr.fit = f[0];

    FILE *file_temp = fopen(path.c_str(), "r");
    if (file_temp != NULL)
    {
        fclose(file_temp);
        throw Exception("disk already created");
    }

    FILE *file = fopen(path.c_str(), "wb");

    if (file == NULL)
    {
        string command_make = "mkdir -p \"" + path + "\"";
        string command_remove = "rmdir \"" + path + "\"";

        if (system(command_make.c_str()) != 0)
            throw Exception("could'nt create path");

        system(command_remove.c_str());

        file = fopen(path.c_str(), "wb");
    }

    fwrite("\0", 1, 1, file);
    fseek(file, size_int - 1, SEEK_SET);
    fwrite("\0", 1, 1, file);
    rewind(file);
    fwrite(&mbr, sizeof(MBR), 1, file);

    fclose(file);

    return 0;
}

int rmdisk(string path)
{
    FILE *file_temp = fopen(path.c_str(), "r");
    if (file_temp == NULL)
        throw Exception("specified disk does not exist");

    string command_remove = "rm \"" + path + "\"";

    if (system(command_remove.c_str()) != 0)
        throw Exception("couldn't remove disk");

    fclose(file_temp);
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

    if (add.empty() && delete.empty())
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

        int size_int = stoi(size);

        if (u == "k")
            size_int = size_int * 1024;
        else if (u == "m")
            size_int = size_int * 1024 * 1024;

        if (size_int < 0)
            throw Exception("partition size was too big");

        if (type == "p")
        {
            Partition *partition = getNewPatition(mbr, f);
            if (partition == NULL)
                throw Exception("disk already has 4 primary partitions");

            strcpy(partition->name, name.c_str());
            partition->status = '0';
            partition->type = type;
            partition->size = size;
        }
        else if (type == "e")
        {
            Partition *partition = getNewPatition(mbr, f);
            if (partition == NULL)
                throw Exception("disk already has 4 primary partitions");

            for (Partition par : mbr.partitions)
                if (par.name == name)
                    throw Exception("partition name in use");

            if (mbr.partition_1.type == 'e' || mbr.partition_2.type == 'e' || mbr.partition_3.type == 'e' || mbr.partition_4.type == 'e' ||)
                throw Exception("disk already has an extended partition");

            strcpy(partition->name, name.c_str());
            partition->type = type;
            partition->size = size;

            EBR ebr;
            ebr.name[0] = '\0';
            ebr.status = '0';
            ebr.size = -1;
            ebr.start = -1;
            ebr.next = -1;
            ebr.fit = '-';

            fseek(file, partition->start, SEEK_SET);
            fwrite(&ebr, sizeof(ebr), 1, file);
        }
        else
        { // LÓGICA
        }
    }
    else if (!add.empty())
    { // RESIZE MODE
        Partition *partition = getPartition(mbr, name.c_str());

        if (partition == NULL)
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

        int size_int = stoi(add);

        if (u == "k")
            size_int = size_int * 1024;
        else if (u == "m")
            size_int = size_int * 1024 * 1024;

        if (size_int < 0)
            throw Exception("disk size was too big");

        if (!isAddable())
            throw Exception("partition can't be grown by " + to_string(add) + u);

        partition->size = partition->size + size_int;
    }
    else
    { // DELETE MODE
        Partition *partition = getPartition(mbr, name.c_str());

        if (partition == NULL)
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

        int size_int = stoi(_delete);

        if (u == "k")
            size_int = size_int * 1024;
        else if (u == "m")
            size_int = size_int * 1024 * 1024;

        if (size_int < 0)
            throw Exception("disk size was too big");

        //validar que se pueda eliminar el espacio
    }

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

    Partition partition = getPartition(mbr, name.c_str());

    if (partition == NULL)
        throw Exception("partition does not exist");

    disk dk = getDiskMounted(path.c_str());

    if (dk == NULL)
    {
        disk *dk;
        dk->id = getDiskId();
        strcpy(dk->path, path.c_str());
        dk->status = 1;
    }

    MountStructs::partition par = getPartitionMounted(dk, name.c_str());

    if (par != NULL)
        throw Exception("partition is already mounted");

    par.id = getPartitionId(dk);
    strcpy(par.name, name.c_str());
    par.status = 1;

    for (int i = 0; i < 26; i++)
        if (dk.partitions[i].name[0] == '\n')
        {
            dk.partitions[i].id = getPartitionId(name.c_str());
            strcpy(dk.partitions[i].name, name.c_str());
            dk.partitions[i].status = 1;
        }

    mounted.push_back(dk);

    return 0;
}

int unmount(string id)
{
    if (id.substr(0, 2) != "98")
        throw Exception("non-valid id (it has to start with '98')");

    if (id.length() < 4 || id.length() > 5)
        throw Exception("non-valid id");

    int disk_i, par_i;

    if (id.length() == 4)
    {
        int *ids = getPartitionMountedByID(id.substr(3, 4)[0], stoi(id.substr(2, 3)));
    
        if (ids == NULL)
            throw Exception("id does not exist");

        disk_i = ids[0]*;
        par_i = ids[1]*;
    }
    else
    {
        int *ids = getPartitionMountedByID(id.substr(4, 5)[0], stoi(id.substr(2, 4)));

        if (ids == NULL)
            throw Exception("id does not exist");

        disk_i = ids[0]*;
        par_i = ids[1]*;
    }

    mounted[disk_i].partitions[par_i].id = 0;
    mounted[disk_i].partitions[par_i].name = "";
    mounted[disk_i].partitions[par_i].status = 0;

    for (MountStructs::partition par : mounted[disk_i].partitions)
        if (par.id != 0)
            return 0;

    mounted.erase(mounted.begin() + ids[0]);
    cout << "disk unmounted" << endl;

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

    //realizar el formateo sobre una particion

    return 0;
}
