#include <iostream>
using namespace std;

int chmod(string path, string ugo, string r = "");

int mkfile(string path, string r = "", string size = "", string cont = "");

int cat(string filen);

int rem(string path);

int edit(string path, string cont);

int ren(string path, string name);

int mkdir(string path, string p = "");

int cp(string path, string dest);

int mv(string path, string mv);

int find(string path, string name);

int chown(string path, string usr, string r = "");

int chgrp(string usr, string grp);
