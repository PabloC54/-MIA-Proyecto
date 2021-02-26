#include <iostream>
#include <fstream>
#include <cstdlib>
#include "time.h"
#include "diskmanager.h"
#include "../Util/util.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"

using namespace std;
vector<disk> mounted;

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
    rewind(file);
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
            Partition partition = getNewPatition(mbr, f);
            if (partition.status == '\\')
                throw Exception("disk already has 4 primary partitions");

            for (Partition par : mbr.partitions)
                if (par.name == name)
                    throw Exception("partition name in use");

            strcpy(partition.name, name.c_str());
            partition.type = type[0];
            partition.size = size_int;
        }
        else if (type == "e")
        {
            Partition partition = getNewPatition(mbr, f);
            if (partition.status == '\\')
                throw Exception("disk already has 4 primary partitions");

            for (Partition par : mbr.partitions)
            {
                if (par.name == name)
                    throw Exception("partition name in use");
                if (par.type == 'e')
                    throw Exception("disk already has an extended partition");
            }

            strcpy(partition.name, name.c_str());
            partition.type = type[0];
            partition.size = size_int;

            EBR ebr;
            ebr.name[0] = '\0';
            ebr.status = '\1';
            ebr.size = -1;
            ebr.start = -1;
            ebr.next = -1;
            ebr.fit = '\0';

            fseek(file, partition.start, SEEK_SET);
            fwrite(&ebr, sizeof(ebr), 1, file);
        }
        else
        { // PARTICIÓN LÓGICA
        }
    }
    else if (!add.empty())
    { // RESIZE MODE
        Partition partition = getPartition(mbr, name.c_str());

        if (partition.status == '\0')
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

        partition.size = partition.size + size_int;
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

        Partition partition = getPartition(mbr, name.c_str());

        if (partition.status != '\0')
        {
            if (partition.type == 'p')
            { // PRIMARIA

                partition.name[0] = '\0';
                partition.status = '\0';
                partition.type = '\0';
                partition.start = -1;
                partition.size = -1;

                if (_delete == "full")
                {
                    // Rellenar con \0
                }
            }
            else if (partition.type == 'e')
            { // EXTENDIDA

                partition.name[0] = '\0';
                partition.status = '\0';
                partition.type = '\0';
                partition.start = -1;
                partition.size = -1;

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
                    partition = par;
                }
            }

            if (partition.name[0] = '\0')
                throw Exception("specified partition does not exist");

            fseek(file, partition.start, SEEK_SET);

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

    if (partition.status == '\0')
        throw Exception("partition does not exist");

    disk dk = getDiskMounted(path.c_str());

    if (dk.status == -1)
    {
        disk dk;
        dk.id = getDiskId();
        strcpy(dk.path, path.c_str());
        dk.status = 1;
    }

    MountStructs::partition par = MountStructs::getPartitionMounted(dk, name.c_str());

    if (par.status != -1)
        throw Exception("partition is already mounted");

    par.id = getPartitionId(dk);
    strcpy(par.name, name.c_str());
    par.status = 1;

    for (int i = 0; i < 26; i++)
        if (dk.partitions[i].status == -1)
        {
            dk.partitions[i].id = getPartitionId(dk);
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
    vector<int> ids;

    if (id.length() == 4)
    {
        ids = getPartitionMountedByID(id.substr(3, 4)[0], stoi(id.substr(2, 3)));

        if (ids.empty())
            throw Exception("id does not exist");

        disk_i = ids[0];
        par_i = ids[1];
    }
    else
    {
        ids = getPartitionMountedByID(id.substr(4, 5)[0], stoi(id.substr(2, 4)));

        if (ids.empty())
            throw Exception("id does not exist");

        disk_i = ids[0];
        par_i = ids[1];
    }

    mounted[disk_i].partitions[par_i].id = 0;
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
