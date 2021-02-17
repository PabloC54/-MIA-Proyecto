#ifndef STRUCTS_H
#define STRUCTS_H

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
    Partition partition_1;
    Partition partition_2;
    Partition partition_3;
    Partition partition_4;
};

struct EBR
{
    char name[16];
    char status;
    int start;
    int size;
    char date[16];
    char fit;
};


class structs
{
public:
    structs();
};

#endif