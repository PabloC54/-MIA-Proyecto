#ifndef PartitionStructs
#define PartitionStructs
#include <vector>

using namespace std;

// ESTRUCTURAS PARA MKFS

struct superblock
{
    int filesystem_type;
    int mnt_count = 0;
    char mtime[16] = {'\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'};
    char umtime[16] = {'\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'};
    int magic = 61267;

    int journal_count = -1;
    int free_journal_count = -1;
    int inodes_count;
    int free_inodes_count;
    int blocks_count;
    int free_blocks_count;

    int journal_size = -1;
    int inode_size;
    int block_size;

    int journal_start = -1;
    int bm_inode_start;
    int bm_block_start;
    int inode_start;
    int block_start;

    int first_journal = -1;
    int first_inode = 0;
    int first_block = 0;
};

struct inode
{
    int uid = 1;
    int gid = 1;
    int size = sizeof(inode);
    char atime[16] = {'\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'};
    char ctime[16] = {'\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'};
    char mtime[16] = {'\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'};
    int block[15] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    char type = '\0';
    int permissions = 664;
};

struct folder_content
{
    char name[12] = {'\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'};
    int inode = -1;
};

struct folder_block
{
    folder_content content[4];
};

struct file_block
{
    char content[64] = {'\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'};
};

struct pointer_block
{
    int pointers[16];
};

struct journal
{
    char operation[10] = "";
    char type = '\\';
    char path[100] = "";
    char content[50] = ""; // Validar cuando espacio necesitan.
    char date[16] = "";
    int size = -1;
};

class partitionstructs
{
public:
    partitionstructs();
};

#endif