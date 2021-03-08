#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "usermanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"
#include "../Util/util.h"

using namespace std;

bool logged;
string user;

int login(string usuario, string password, string id)
{
    if (logged)
        throw Exception("session already active; logout before logging in");

    if (id.length() < 4 || id.length() > 5)
        throw Exception("non-valid id");
    if (id.substr(0, 2) != "98")
        throw Exception("non-valid id (ids start with '98')");

    const char *path;
    string name;
    vector<const char *> data;

    if (id.length() == 4)
        data = getPartitionMountedByID(id.substr(3, 4)[0], stoi(id.substr(2, 3)));
    else
        data = getPartitionMountedByID(id.substr(4, 5)[0], stoi(id.substr(2, 4)));

    if (data.empty())
        throw Exception("id does not exist");

    path = data[1];
    name = data[0];

    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(&mbr, name);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    inode inodo;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    string content = "";

    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content);
    }

    stringstream ss(content);
    string line;

    while (std::getline(ss, line, '\n'))
    {

        stringstream us(line);
        vector<string> words;

        while (us.good())
        {
            string substr;
            getline(ss, substr, ',');
            words.push_back(substr);
        }

        if (words.at(1) == "U")
            if (words.at(3) == usuario)
                if (words.at(4) != password)
                {
                    fclose(file);
                    throw Exception("wrong password");
                }
    }

    logged = true;
    user = usuario;

    fclose(file);

    return 0;
}

int logout()
{
    if (!logged)
        throw Exception("no session active");

    // logout

    user.clear();
    logged = false;

    return 0;
}

int mkgrp(string name)
{
    if (!logged)
        throw Exception("no session active");

    if (user != "root")
        throw Exception("user has no permissions");

    // validar que no exista el nombre. El nombre debe ser case sensitive
    // make group

    return 0;
}

int rmgrp(string name)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user != "root")
        throw Exception("user has no permissions");

    // validar que exista el grupo
    // remove group

    return 0;
}

int mkusr(string usr, string pwd, string grp)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user != "root")
        throw Exception("user has no permissions");

    if (usr.length() > 10)
        throw Exception("parameter -usr non-valid, max. lenght = 10");

    if (pwd.length() > 10)
        throw Exception("parameter -pwd non-valid, max. lenght = 10");

    if (grp.length() > 10)
        throw Exception("parameter -grp non-valid, max. lenght = 10");

    // validar que exista el grupo
    // validar que no exista el usuario

    // make user

    return 0;
}

int rmusr(string usr)
{
    if (!logged)
        throw Exception("no sesion active");

    if (user != "root")
        throw Exception("user has no permissions");

    // validar que exista el usuario
    // remove user

    return 0;
}
