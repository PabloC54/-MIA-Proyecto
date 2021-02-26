#include "diskstructs.h"

#include <cstddef>
#include <iostream>
#include <cstring>

using namespace std;
diskstructs::diskstructs()
{
}

Partition getPartition(MBR mbr, const char *name)
{
    Partition par_temp;

    for (Partition par : mbr.partitions)
        if (par.name == name)
            return par;

    return par_temp;
}

Partition getNewPatition(MBR mbr, string fit)
{
    Partition par;
    par.status = '\\';

    if (fit == "ff")
    {
        if (mbr.partitions[0].status == 'e')
        {
            par = mbr.partitions[0];
            strcpy(par.fit, fit.c_str());
            par.start = sizeof(MBR);
        }
        else if (mbr.partitions[1].status == 'e')
        {
            par = mbr.partitions[1];
            strcpy(par.fit, fit.c_str());
            par.start = mbr.partitions[0].start + mbr.partitions[0].size;
        }
        else if (mbr.partitions[2].status == 'e')
        {
            par = mbr.partitions[2];
            strcpy(par.fit, fit.c_str());
            par.start = mbr.partitions[1].start + mbr.partitions[1].size;
        }
        else if (mbr.partitions[3].status == 'e')
        {
            par = mbr.partitions[3];
            strcpy(par.fit, fit.c_str());
            par.start = mbr.partitions[2].start + mbr.partitions[2].size;
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
