#include <cstddef>
#include <iostream>

#ifndef DiskStructs
#define DiskStructs

using namespace std;

struct Partition
{
    char name[16];
    char status;
    char type;
    char fit;
    int start;
    int size;
};

struct MBR
{
    int size;
    char date[16];
    int signature;
    char fit;
    Partition partitions[4];
};

struct EBR
{
    char name[16];
    char status;
    int start;
    int size;
    int next;
    char fit;
};

Partition getPartition(MBR mbr, char name[])
{
    Partition par_temp;

    for(Partition par:mbr.partitions)

    if (par.name == name)
        par_temp = par;
        

    return par_temp;
}

Partition *getNewPatition(MBR mbr, string fit)
{
    Partition *par = NULL;

    if (fit == "ff")
    {
        if (mbr.partitions[0].name[0] == '\0')
        {
            par = &mbr.partitions[0];
            par->fit = fit[0];
            par->start = sizeof(MBR);
        }
        else if (mbr.partitions[1].name[0] == '\0')
        {
            par = & mbr.partitions[1];
            par->fit = fit[0];
            par->start = mbr.partitions[0].start + mbr.partitions[0].size;
        }
        else if (mbr.partitions[2].name[0] == '\0')
        {
            par = & mbr.partitions[2];
            par->fit = fit[0];
            par->start = mbr.partitions[1].start + mbr.partitions[1].size;
        }
        else if (mbr.partitions[3].name[0] == '\0')
        {
            par = & mbr.partitions[3];
            par->fit = fit[0];
            par->start = mbr.partitions[2].start + mbr.partitions[2].size;
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

bool isAddable(){


    return true;
}

class diskstructs
{
public:
    diskstructs();
};

#endif