#ifndef UserManager
#define UserManager

#include <iostream>
using namespace std;

extern bool logged;
extern string user_logged;
extern int user_num;
extern int group_logged;
extern const char* disk_path;
extern string partition_name;

int login(string, string, string);

int logout();

int mkgrp(string);

int rmgrp(string);

int mkusr(string, string, string);

int rmusr(string);

#endif