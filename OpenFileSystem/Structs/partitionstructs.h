#ifndef PartitionStructs
#define PartitionStructs
#include <vector>

using namespace std;

// ESTRUCTURAS PARA MKFS

struct superblock
{
    int filesystem_type;
    int inodes_count;
    int free_inodes_count;
    int blocks_count;
    int free_blocks_count;
    char mtime[16];
    char umtime[16];
    int mnt_count;
    int magic;
    int inode_size;
    int block_size;
    int first_inode;
    int frist_block;
    int bm_inode_start;
    int bm_block_start;
    int inode_start;
    int block_start;
};

struct inode
{
    int uid;
    int gid;
    int size;
    char atime[16];
    char ctime[16];
    char mtime[16];
    int block[12];
    char type;
    int permissions;
};

struct folder_content
{
    char name[12];
    int inode;
};

struct folder_block
{
    folder_content content[4];
};

struct file_block
{
    char content[64];
};

struct pointer_block
{
    int pointers[16];
};

struct journal
{
    char operation[10] = "";
    char type = '\0';
    char path[100] = "";
    char content[50] = ""; // Validar cuando espacio necesitan.
    char date[16] = "";
    int size;
};

class partitionstructs
{
public:
    partitionstructs();
};

#endif