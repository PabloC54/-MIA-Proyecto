#ifndef Util
#define Util

#include <iostream>
#include <regex>
#include <boost/algorithm/string.hpp>

#include "../Managers/usermanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"

using namespace std;

string unquote(string);

int get_first_inode(FILE *, superblock);

int get_first_block(FILE *, superblock);

int create_journal(FILE *, superblock *, const char *, string, string, string, int, char);

string read_inode_content(FILE *, superblock, inode);

int write_inode_content(FILE *, superblock *, inode *, string);

int new_folder(FILE *, superblock *, inode *, int, string, const char *);

int new_file(FILE *, superblock *, inode *, int, string, const char *);

bool exists_inode(FILE *, superblock *, inode *, string);

int get_inode(FILE *, superblock *, inode *, string);

bool has_permission(inode, const char *);

int change_permission(FILE *, superblock, int, string, const char *);

int change_owner(FILE *, superblock, int, int, const char *);

bool is_deleteable(FILE *, superblock, inode);

int delete_inode(FILE *, superblock, inode);

int copy_inode(FILE *, superblock *, inode);

string find_inode(FILE *, superblock, inode, string, int, regex);

class Exception : public std::runtime_error
{
public:
    Exception(const char *what) : runtime_error(what) {}
};

#endif