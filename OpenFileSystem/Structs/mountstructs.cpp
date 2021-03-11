
#include <vector>
#include "mountstructs.h"
#include <string.h>
#include <string>

mountstructs::mountstructs()
{
}

using namespace std;

char L[26];
int D[99];

void init()
{
    for (int ch = 'A'; ch <= 'Z'; ch++)
        L[ch - 'A'] = ch;

    for (int d = 1; d <= 99; d++)
        D[d - 1] = d;
}

vector<const char *> getPartitionMountedByID(char disk_id, int par_id)
{
    vector<const char *> data;

    for (int i = 0; i < 26; i++)
        if (mounted[i].id == disk_id)
            for (int j = 0; j < 99; j++)
                if (mounted[i].partitions[j].id == par_id)
                {
                    data.insert(data.begin(), mounted[i].path);
                    data.insert(data.begin(), mounted[i].partitions[j].name);
                }

    return data;
}

char getDiskId()
{
    init();
    char temp_id;

    for (char id : L)
    {
        bool br = false;
        for (disk dk : mounted)
            if (dk.id == id)
            {
                br = true;
                break;
            }

        if (br)
            continue;

        temp_id = id;
        break;
    };

    return temp_id;
}

int getPartitionId(disk dk)
{
    init();
    int id_temp;

    for (int id : D)
    {
        bool br = false;
        for (MountStructs::partition par : dk.partitions)
            if (par.id == id)
            {
                br = true;
                break;
            }

        if (br)
            continue;

        id_temp = id;
        break;
    }

    return id_temp;
}
