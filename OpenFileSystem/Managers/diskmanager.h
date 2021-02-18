#ifndef DiskManager
#define DiskManager

#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;

int mkdisk(string, string, string, string);

int rmdisk(string);

int fdisk(string, string, string, string, string, string, string, string);

int mount(string, string);

int unmount(string);

int mkfs(string, string, string);

#endif