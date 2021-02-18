#ifndef Structs
#define Structs

using namespace std;

// ESTRUCTURAS PARA MOUNT

// 98<num_particion><letra_disco>

struct partition
{
    int id;
    char name[16];
    int status

}

struct disk
{
    char id;
    char path[100];
    partition partitions[99];
    int status;
}

disk mounted[26];

char *[2] searchMount(char disk_id, int partition_id) {
    for (disk : mounted)
        if (disk.id == disk_id)

            for (partition : disk.partitions)
                if (partition.id == partition_id)
                    return pair(disk.path, partition.name);

    return {};
}

char L[26];

for (int ch = 'a'; ch <= 'z'; ch++)
{
    L[ch - 'a'] = ch;

    char *[2] getMountId() {
        for (disk : mounted)
            if (disk.id == disk_id)

                for (partition : disk.partitions)
                    if (partition.id == partition_id)
                        return pair(disk.path, partition.name);

        return {};
    }

    class mountstructs
    {
    public:
        mountstructs();
    };

#endif