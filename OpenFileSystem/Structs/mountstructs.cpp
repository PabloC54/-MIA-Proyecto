
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

// disk *getDiskMounted(const char *path)
// {
//     MountStructs::disk *dk_temp;
//     disk new_disk;
//     dk_temp = &new_disk;

//     for (disk dk : mounted)
//         if (strncmp(dk.path, path, strlen(path)) == 0)
//         {
//             return &dk;
//         }

//     return dk_temp;
// }

// MountStructs::partition *getPartitionMounted(disk *dk, const char *name)
// {
//     MountStructs::partition *par_temp;
//     partition new_partition;
//     par_temp = &new_partition;

//     for (MountStructs::partition par : dk->partitions)
//         if (strncmp(par.name, name, strlen(name)) == 0)
//         {
//             return &par;
//         }

//     return par_temp;
// }

// MountStructs::partition *getFreePartition(disk *dk, const char *name)
// {
//     MountStructs::partition *par_temp;
//     partition new_partition;
//     par_temp = &new_partition;

//     for (MountStructs::partition par : dk->partitions)
//         if (par.status == -1)
//         {
//             par.status = 1;
//             return &par;
//         }

//     return par_temp;
// }

bool isMounted(disk *dk, const char *name)
{
    MountStructs::partition *par_temp;
    partition new_partition;
    par_temp = &new_partition;

    for (MountStructs::partition par : dk->partitions)
        if (string(par.name) == name)
            return true;

    return false;
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
