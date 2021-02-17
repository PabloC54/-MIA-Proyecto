#include <iostream>
#include <fstream>
#include <cstdlib>
#include "diskmanager.h"
using namespace std;

int mkdisk(string size, string f, string u, string path)
{
    // time_t t;
    // struct tm *tm;
    // char fechayhora[20];

    // t = time(NULL);
    // tm = localtime(&t);
    // strftime(fechayhora, 20, "%Y/%m/%d %H:%M:%S", tm);

    // if (unit == "")
    // {
    //     unit = "M";
    // }

    // int numSize = stoi(size);

    // if (unit == "M" || unit == "m")
    // {
    //     numSize = numSize * 1024 * 1024;
    // }
    // else
    // {
    //     numSize = numSize * 1024;
    // }

    // if (numSize < 0)
    // {
    //     cout << "Error, tamanio del disco demasiado grande." << endl;
    //     return;
    // }

    // Structs::MBR disco;
    // disco.size = numSize;
    // strcpy(disco.date, fechayhora);
    // disco.sign = rand() % 100;

    // FILE *validar = fopen(path.c_str(), "r");
    // if (validar != NULL)
    // {
    //     cout << "Disco ya existente., validar path" << endl;
    //     fclose(validar);
    //     return;
    // }
    // //cout << "Nuevo disco: \nSize: " << disco.size << "\ndate: " << disco.date << "\nFit: " << disco.fit << "\nDisk_Signature: " << disco.disk_signature << endl;
    // // cout << "Bits del MBR: " << sizeof(Structs::MBR) << endl;

    // // cout << "Path: " << path << endl;
    // int isNulo = 0;
    // FILE *bfile = fopen(path.c_str(), "wb");
    // if (bfile != NULL)
    // {
    //     fwrite("\0", 1, 1, bfile);
    //     fseek(bfile, numSize - 1, SEEK_SET);
    //     fwrite("\0", 1, 1, bfile);
    //     rewind(bfile);
    //     fwrite(&disco, sizeof(Structs::MBR), 1, bfile);
    //     cout << "Disco creado exitosamente" << endl;
    // }
    // else
    // {
    //     isNulo = 1;
    //     cout << "Path creado exitosamente" << endl;
    //     //Creo la carpeta de la direccion.
    //     string comando1 = "mkdir -p \"" + path + "\"";
    //     string comando2 = "rmdir \"" + path + "\"";
    //     system(comando1.c_str());
    //     system(comando2.c_str());

    //     bfile = fopen(path.c_str(), "wb");
    //     fwrite("\0", 1, 1, bfile);
    //     fseek(bfile, numSize - 1, SEEK_SET);
    //     fwrite("\0", 1, 1, bfile);
    //     rewind(bfile);
    //     fwrite(&disco, sizeof(Structs::MBR), 1, bfile);
    //     cout << "Disco creado exitosamente" << endl;
    // }
    // fclose(bfile);

    return 0;
}

int rmdisk(string path)
{

    return 0;
}

int fdisk(string size, string u, string path, string type, string f, string _delete, string name,  string add)
{

    return 0;
}

int mount(string path, string name)
{

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
