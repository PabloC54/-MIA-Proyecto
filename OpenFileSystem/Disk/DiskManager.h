#include <iostream>
using namespace std;

int mkdisk(string size, string f = "", string u = "", string path = "");

int rmdisk(string path);

int fdisk(string size, string path, string name, string u = "", string type = "", string f = "", string _delete = "", string add = "");

int mount(string path, string name);

int unmount(string id);

int mkfs(string id, string type = "", string fs = "");
