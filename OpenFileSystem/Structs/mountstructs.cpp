
#include <vector>
#include "mountstructs.h"

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

disk getDiskMounted(const char *path)
{
    MountStructs::disk dk_temp;

    for (disk dk : mounted)
        if (dk.path == path)
            dk_temp = dk;

    return dk_temp;
}

MountStructs::partition getPartitionMounted(disk dk, const char *name)
{
    MountStructs::partition par_temp;

    for (MountStructs::partition par : dk.partitions)
        if (par.name == name)
            par_temp = par;

    return par_temp;
}

vector<int> getPartitionMountedByID(char disk_id, int par_id)
{
    vector<int> par;

    for (int i = 0; i < 26; i++)
        if (mounted[i].id == disk_id)
            for (int j = 0; j < 99; j++)
                if (mounted[i].partitions[j].id == par_id)
                {
                    par.insert(par.begin(), i);
                    par.insert(par.begin(), j);
                }

    return par;
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
