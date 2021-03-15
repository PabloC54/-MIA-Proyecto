#ifndef StorageManager
#define StorageManager

#include "../Structs/partitionstructs.h"
#include <iostream>
#include <map>

using namespace std;

string read_inode_content(FILE *, superblock, inode);

int write_inode_content(FILE *, superblock *, inode *, string);

int chmod(string, string, string);

//int change_permission(FILE, superblock, inode, int, string, char *);

int mkfile(string, string, string, string);

int cat(map<string, string>);

int rem(string);

int edit(string, string);

int ren(string, string);

int mkdir(string, string);

int cp(string, string);

int mv(string, string);

int find(string, string);

int chown(string, string, string);

int chgrp(string, string);

int get_first_inode(FILE *, superblock);

int get_first_block(FILE *, superblock);

#endif