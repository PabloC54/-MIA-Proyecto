#ifndef StorageManager
#define StorageManager

#include "../Structs/partitionstructs.h"
#include <iostream>
#include <map>

using namespace std;

int chmod(string, string, string);

int change_permission(FILE, superblock, inode, int, string, char *);

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

#endif