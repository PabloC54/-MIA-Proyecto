#include <cstddef>
#include <iostream>
#include <cstring>

#ifndef DiskStructs
#define DiskStructs

using namespace std;

struct Partition
{
    char name[16];
    char status = 'e';
    char type = '\0';
    char fit[2] = {'\0', '\0'};
    int start = -1;
    int size = -1;
};

struct MBR
{
    int size;
    char date[16];
    int signature;
    char fit[2];
    Partition partitions[4];
};

struct EBR
{
    char name[16];
    char status;
    int start;
    int size;
    int next = -1;
    char fit;
};

Partition getPartition(MBR, const char *);

Partition getNewPatition(MBR, string);

bool isResizable(MBR, string, int);

class diskstructs
{
public:
    diskstructs();
};

#endif