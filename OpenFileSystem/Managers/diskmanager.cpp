#include <iostream>
#include <fstream>
#include <cstdlib>
#include "time.h"
#include "diskmanager.h"
#include "../Util/util.h"
#include "../Structs/structs.h"
using namespace std;

int mkdisk(string size, string f, string u, string path)
{
    if (f.empty())
        f = "ff";
    if (u.empty())
        u = "m";

    if (f != "bf" && f != "ff" && f != "wf")
        throw Exception("-f parameter non-valid. Valid: bf (best fit), ff (first fit), wf (worst fit)");

    int units;

    if (u == "k")
        units = 1024;
    else if (u == "m")
        units = 1024 * 1024;
    else
        throw Exception("-u parameter non-valid. Valid: k (kilobytes), m (megabytes)");

    int size_int = stoi(size);

    if (u == "k")
        size_int = size_int * 1024;
    else
        size_int = size_int * 1024 * 1024;

    if (size_int < 0)
        throw Exception("disk size was too big");

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char date[16];
    strftime(date, 16, "%d/%m/%Y %H:%M", tm);

    MBR mbr;
    mbr.size = size_int;
    strcpy(mbr.date, date);
    mbr.signature = rand() % 100;
    mbr.fit = f[0];

    FILE *file_temp = fopen(path.c_str(), "r");
    if (file_temp != NULL)
    {
        fclose(file_temp);
        throw Exception("disk already created");
    }

    FILE *file = fopen(path.c_str(), "wb");

    if (file == NULL)
    {
        string command_make = "mkdir -p \"" + path + "\"";
        string command_remove = "rmdir \"" + path + "\"";

        if (system(command_make.c_str()) != 0)
            throw Exception("could'nt create path");

        system(command_remove.c_str());

        file = fopen(path.c_str(), "wb");
    }

    fwrite("\0", 1, 1, file);
    fseek(file, size_int - 1, SEEK_SET);
    fwrite("\0", 1, 1, file);
    rewind(file);
    fwrite(&mbr, sizeof(MBR), 1, file);

    fclose(file);

    return 0;
}

int rmdisk(string path)
{
    FILE *file_temp = fopen(path.c_str(), "r");
    if (file_temp == NULL)
        throw Exception("specified disk does not exist");

    string command_remove = "rm \"" + path + "\"";

    if (system(command_remove.c_str()) != 0)
        throw Exception("couldn't remove disk");

    fclose(file_temp);
    return 0;
}

int fdisk(string size, string u, string path, string type, string f, string _delete, string name, string add)
{
    if (u.empty())
        u = "k";
    if (f.empty())
        f = "wf";
    if (type.empty())
        type = "p";

    if (!_delete.empty() && !add.empty())
        throw Exception("-add, -delete parameters are incompatible");

    if (f != "bf" && f != "ff" && f != "wf")
        throw Exception("-f parameter non-valid. Valid: bf (best fit), ff (first fit), wf (worst fit)");

    if (type != "p" && type != "e" && type != "l")
        throw Exception("-type parameter non-valid. Valid: p (primary), e (extended), l (logical)");

    // hacer validaciones de particiones
    // extendidas = 1
    // cant_primariass + extendidas <= 4
    // no se deben sobrepasar limites
    // particiones logicas se crean si existe extendida, no debe sobrepasar el tamaño de esta

    // validar name no repetido

    if (add.empty() && add.empty())
    { // CREATE MODE
        if (size.empty())
            throw Exception("-size parameter is needed when creating a partition");

        FILE *file_temp = fopen(path.c_str(), "r");
        if (file_temp != NULL)
        {
            fclose(file_temp);
            throw Exception("disk already exists");
        }

        int units;

        if (u == "b")
            units = 1;
        else if (u == "k")
            units = 1024;
        else if (u == "m")
            units = 1024 * 1024;
        else
            throw Exception("-u parameter non-valid. Valid: b (bytes), k (kilobytes), m (megabytes)");

        int size_int = stoi(size);

        if (u == "k")
            size_int = size_int * 1024;
        else if (u == "m")
            size_int = size_int * 1024 * 1024;

        if (size_int < 0)
            throw Exception("disk size was too big");

        Partition par;
        // par.position = saber;
        par.size = size_int;
        par.fit = 32;

        // validar los fit
    }
    else if (!add.empty())
    { // ADD MODE
        FILE *file_temp = fopen(path.c_str(), "r");
        if (file_temp == NULL)
            throw Exception("specified disk does not exist");
        // validar que exista la particion

        int units;

        if (u == "b")
            units = 1;
        else if (u == "k")
            units = 1024;
        else if (u == "m")
            units = 1024 * 1024;
        else
            throw Exception("-u parameter non-valid. Valid: b (bytes), k (kilobytes), m (megabytes)");

        int size_int = stoi(add);

        if (u == "k")
            size_int = size_int * 1024;
        else if (u == "m")
            size_int = size_int * 1024 * 1024;

        if (size_int < 0)
            throw Exception("disk size was too big");
    }
    else
    { // DELETE MODE
        FILE *file_temp = fopen(path.c_str(), "r");
        if (file_temp == NULL)
            throw Exception("specified disk does not exist");
        // validar que exista la particion
    }

    int isNulo = 0;
    FILE *file = fopen(path.c_str(), "wb");

    if (file == NULL)
    {
        //Creo la carpeta de la direccion.
        string command_make = "mkdir -p \"" + path + "\"";
        string command_remove = "rmdir \"" + path + "\"";

        if (system(command_make.c_str()) != 0)
            throw Exception("could'nt create path");

        system(command_remove.c_str());

        file = fopen(path.c_str(), "wb");
    }

    // fwrite("\0", 1, 1, file);
    // fseek(file, size_int - 1, SEEK_SET);
    // fwrite("\0", 1, 1, file);
    // rewind(file);
    // fwrite(&mbr, sizeof(MBR), 1, file);

    fclose(file);

    return 0;
}

int mount(string path, string name)
{

    if (path.empty() && path.empty())
    {
        // STRUCTS_H::print();
        return 0;
    }

    // hacer el montaje

    return 0;
}

int unmount(string id)
{

    return 0;
}

int mkfs(string id, string type, string fs)
{

    return 0;
}
