#ifndef MountStructs
#define MountStructs
#include <vector>

using namespace std;

extern char L[26];
extern int D[99];

void init();

// ESTRUCTURAS PARA MOUNT

// 98<num_particion><letra_disco>

struct partition
{
    int id = -1;
    char name[16];
    int status = -1;
};

struct disk
{
    char id;
    char path[100];
    struct partition partitions[99];
    int status = -1;
};

extern vector<disk> mounted;

disk *getDiskMounted(const char *);

MountStructs::partition *getPartitionMounted(disk *, const char *);

vector<const char *> getPartitionMountedByID(char, int);

char getDiskId();

int getPartitionId(disk);

class mountstructs
{
public:
    mountstructs();
};

#endif