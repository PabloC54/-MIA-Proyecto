#ifndef UserManager
#define UserManager

#include <iostream>
using namespace std;

extern bool logged;
extern string user;
extern string password;

int login(string, string, string);

int logout();

int mkgrp(string);

int rmgrp(string);

int mkusr(string, string, string);

int rmusr(string);

#endif