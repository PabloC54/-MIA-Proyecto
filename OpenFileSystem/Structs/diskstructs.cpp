#include "diskstructs.h"
#include "../Util/util.h"

#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace std;
diskstructs::diskstructs()
{
}

Partition *getPartition(FILE *file, MBR *mbr, string name)
{
    Partition *par_temp;
    Partition new_partition;
    par_temp = &new_partition;

    bool found = false;

    for (int i = 0; i < 4; i++)
    {
        if (string(mbr->partitions[i].name) == name)
        {
            par_temp = &mbr->partitions[i];
            found = true;
            break;
        }
    }

    if (!found)
    {
        for (int i = 0; i < 4; i++)
        {
            if (mbr->partitions[i].type == 'e')
            {
                fseek(file, mbr->partitions[i].start, SEEK_SET);
                EBR ebr;
                fread(&ebr, sizeof(EBR), 1, file);

                while (true)
                {
                    if (string(ebr.name) == name)
                    {
                        strcpy(par_temp->name, ebr.name);
                        par_temp->start = ebr.start;
                        par_temp->size = ebr.size;
                        par_temp->status = ebr.status;
                        strcpy(par_temp->fit, ebr.fit);
                        par_temp->type = 'l';
                        break;
                    }

                    if (ebr.next != -1)
                    {
                        fseek(file, ebr.next, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, file);
                    }
                    else
                        break;
                }

                break;
            }
        }
    }

    return par_temp;
}

Partition *getNewPatition(MBR *mbr, string fit, int size)
{
    Partition *par;
    Partition new_partition;
    par = &new_partition;

    bool found = false;
    vector<int> starts;

    for (int i = 0; i < 4; i++)
        if (mbr->partitions[i].status == '\\')
        {
            if (found)
                continue;

            par = &mbr->partitions[i];
            found = true;
        }
        else
        {
            starts.push_back((mbr->partitions[i].start));
        }

    if (!found)
        throw Exception("disk already has 4 primary partitions");

    sort(starts.begin(), starts.end());

    vector<vector<int>> occuppied;

    for (int start : starts)
    {
        for (int i = 0; i < 4; i++)
        {
            if (mbr->partitions[i].start == start)
            {
                vector<int> temp;
                temp.push_back(i);
                temp.push_back(start);
                temp.push_back(mbr->partitions[i].size);

                occuppied.push_back(temp);
            }
        }
    }

    vector<vector<int>> blank;

    if (occuppied.empty())
    {
        vector<int> temp;
        temp.push_back(sizeof(MBR));
        temp.push_back(mbr->size);
        blank.push_back(temp);
    }
    else
    {
        if (sizeof(MBR) < occuppied.at(0).at(1))
        {
            vector<int> temp;
            temp.push_back(sizeof(MBR));
            temp.push_back(occuppied.at(0).at(1));
            blank.push_back(temp);
        }

        for (int i = 0; i < occuppied.size(); i++)
        {
            vector<int> temp;

            if (i != occuppied.size() - 1)
            {
                if (occuppied.at(i).at(1) + occuppied.at(i).at(2) < occuppied.at(i + 1).at(1))
                {
                    temp.push_back(occuppied.at(i).at(1) + occuppied.at(i).at(2));
                    temp.push_back(occuppied.at(i + 1).at(1));
                    blank.push_back(temp);
                }
            }
            else if (occuppied.at(i).at(2) < mbr->size)
            {
                temp.push_back(occuppied.at(i).at(1) + occuppied.at(i).at(2));
                temp.push_back(mbr->size);
                blank.push_back(temp);
            }
        }
    }

    strcpy(par->fit, fit.c_str());
    found = false;

    if (fit == "ff")
    {
        for (vector<int> b : blank)
            if (b.at(1) - b.at(0) >= size)
            {
                par->start = b.at(0);
                found = true;
                break;
            }
    }
    else if (fit == "bf")
    {
        int min = 100000000;
        for (vector<int> b : blank)
            if ((b.at(1) - b.at(0)) >= size && ((b.at(1) - b.at(0)) - size) < min)
            {
                par->start = b.at(0);
                found = true;
                min = (b.at(1) - b.at(0)) - size;
            }
    }
    else if (fit == "wf")
    {
        int max = 0;
        for (vector<int> b : blank)
            if ((b.at(1) - b.at(0)) >= size && ((b.at(1) - b.at(0)) - size) > max)
            {
                par->start = b.at(0);
                found = true;
                max = (b.at(1) - b.at(0)) - size;
            }
    }

    if (!found)
        throw Exception("could not fit partition, try with a smaller size");

    par->status = '0';
    par->size = size;

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
            int end = mbr.partitions[3].start + mbr.partitions[3].size + size;
            if (end < mbr.size)
                return true;
        }
    }
    else
    { // QUITAR TAMAÑO
        if (mbr.partitions[0].name == name.c_str())
        {
            int end = mbr.partitions[0].size + size;
            if (end >= 2000) //tamaño necesario para mkfs
                return true;
        }
        else if (mbr.partitions[1].name == name.c_str())
        {
            int end = mbr.partitions[1].size + size;
            if (end > 2000) //tamaño necesario para mkfs
                return true;
        }
        else if (mbr.partitions[2].name == name.c_str())
        {
            int end = mbr.partitions[2].size + size;
            if (end > 2000) //tamaño necesario para mkfs
                return true;
        }
        else if (mbr.partitions[3].name == name.c_str())
        {
            int end = mbr.partitions[3].size + size;
            if (end > 2000) //tamaño necesario para mkfs
                return true;
        }
    }

    return false;
}
