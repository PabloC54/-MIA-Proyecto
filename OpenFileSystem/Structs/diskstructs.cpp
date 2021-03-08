#include "diskstructs.h"

#include <cstddef>
#include <iostream>
#include <cstring>

using namespace std;
diskstructs::diskstructs()
{
}

Partition *getPartition(MBR *mbr, string name)
{
    Partition *par_temp;
    Partition new_partition;
    par_temp = &new_partition;

    for (int i = 0; i < 4; i++)
    {
        if (string(mbr->partitions[i].name) == name)
        {
            par_temp = &mbr->partitions[i];
            break;
        }
    }

    return par_temp;
}

Partition *getNewPatition(MBR *mbr, string fit)
{
    Partition *par;
    Partition new_partition;
    par = &new_partition;

    if (fit == "ff")
    {
        if (mbr->partitions[0].status == '\\')
        {
            par = &mbr->partitions[0];
            strcpy(par->fit, fit.c_str());
            par->start = sizeof(MBR);
            par->status = '\0';
        }
        else if (mbr->partitions[1].status == '\\')
        {
            par = &mbr->partitions[1];
            strcpy(par->fit, fit.c_str());
            par->start = mbr->partitions[0].start + mbr->partitions[0].size;
            par->status = '\0';
        }
        else if (mbr->partitions[2].status == '\\')
        {
            par = &mbr->partitions[2];
            strcpy(par->fit, fit.c_str());
            par->start = mbr->partitions[1].start + mbr->partitions[1].size;
            par->status = '\0';
        }
        else if (mbr->partitions[3].status == '\\')
        {
            par = &mbr->partitions[3];
            strcpy(par->fit, fit.c_str());
            par->start = mbr->partitions[2].start + mbr->partitions[2].size;
            par->status = '\0';
        }
    }
    else if (fit == "bf")
    {
    }
    else if (fit == "wf")
    {
    }

    return par;
}

bool isResizable(MBR mbr, string name, int size)
{
    if (size > 0)
    { // AGREGAR TAMAÑO
        if (mbr.partitions[0].name == name.c_str())
        {
            int end = mbr.partitions[0].start + mbr.partitions[0].size + size;
            if (end < mbr.partitions[1].start)
                return true;
        }
        else if (mbr.partitions[1].name == name.c_str())
        {
            int end = mbr.partitions[1].start + mbr.partitions[1].size + size;
            if (end < mbr.partitions[2].start)
                return true;
        }
        else if (mbr.partitions[2].name == name.c_str())
        {
            int end = mbr.partitions[2].start + mbr.partitions[2].size + size;
            if (end < mbr.partitions[3].start)
                return true;
        }
        else if (mbr.partitions[3].name == name.c_str())
        {
            int end = mbr.partitions[0].start + mbr.partitions[0].size + size;
            if (end < mbr.size)
                return true;
        }
    }
    else
    { // QUITAR TAMAÑO
        if (mbr.partitions[0].name == name.c_str())
        {
            int end = mbr.partitions[0].size + size;
            if (end > 100) //tamaño necesario para mkfs
                return true;
        }
        else if (mbr.partitions[1].name == name.c_str())
        {
            int end = mbr.partitions[1].size + size;
            if (end > 100) //tamaño necesario para mkfs
                return true;
        }
        else if (mbr.partitions[2].name == name.c_str())
        {
            int end = mbr.partitions[2].size + size;
            if (end > 100) //tamaño necesario para mkfs
                return true;
        }
        else if (mbr.partitions[3].name == name.c_str())
        {
            int end = mbr.partitions[3].size + size;
            if (end > 100) //tamaño necesario para mkfs
                return true;
        }
    }

    return false;
}
