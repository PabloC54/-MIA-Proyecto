#ifndef UserManager
#define UserManager

#include <iostream>
using namespace std;

int login(string usuario, string password, string id);

int logout();

int mkgrp(string name);

int rmgrp(string name);

int mkusr(string usr, string pwd, string grp);

int rmusr(string usr);

#endif