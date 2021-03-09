#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "reportmanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"
#include "../Util/util.h"

using namespace std;

string rep_mbr(const char *path, MBR mbr)
{
    string s = "";

    s += " digraph G {\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";
    s += +" Foo [label=<\n";
    s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    s += +"   <tr><td><i>Estado</i></td><td><i>Siguientes</i></td></tr>\n";

    s += "<tr><td align=\"left\">Nombre</td><td>" + string(path) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Tamaño</td><td>" + to_string(mbr.size) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fecha de creación</td><td>" + string(mbr.date) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Signature</td><td>" + to_string(mbr.signature) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fit</td><td>" + string(mbr.fit) + "</td></tr>\n";

    for (int i = 0; i < 4; i++)
    {
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Nombre</td><td>" + (mbr.partitions[i].name) + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Estado</td><td>" + mbr.partitions[i].status + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Tipo</td><td>" + mbr.partitions[i].type + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Fit</td><td>" + mbr.partitions[i].fit + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Inicio</td><td>" + to_string(mbr.partitions[i].start) + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Tamaño</td><td>" + to_string(mbr.partitions[i].size) + "</td></tr>\n";
    }

    s += "</table>>];";
    s += "} ";

    return s;
}

int rep(string name, string path, string id, string ruta)
{
    transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name != "mbr" && name != "disk" && name != "inode" && name != "journaling" && name != "block" && name != "bm_inode" && name != "bm_block" && name != "tree" && name != "sb" && name != "file" && name != "ls")
        throw Exception("-mbr parameter non-valid. Valid: mbr, disk, inode, journaling, block, bm_inode, bm_block, tree, sb, file, ls");

    string command_make = "mkdir -p \"" + path + "\"";
    string command_remove = "rmdir \"" + path + "\"";

    if (system(command_make.c_str()) != 0)
        throw Exception("could not create path");

    system(command_remove.c_str());

    vector<const char *> data;

    if (true)
        if (id.length() == 4)
            data = getPartitionMountedByID(id.substr(3, 4)[0], stoi(id.substr(2, 3)));
        else
            data = getPartitionMountedByID(id.substr(4, 5)[0], stoi(id.substr(2, 4)));

    if (data.empty())
        throw Exception("id does not exist");

    const char *disk_path = data[1];
    string partition_name = data[0];

    FILE *file = fopen(disk_path, "rb");
    fseek(file, 0, SEEK_SET);

    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    int return_value = 0;
    string body;

    if (name == "mbr")
    {
        body = rep_mbr(disk_path, mbr);
    }
    else if (name == "disk")
    {
    }
    else if (name == "inode")
    {
    }
    else if (name == "journaling")
    {
    }
    else if (name == "block")
    {
    }
    else if (name == "bm_inode")
    {
    }
    else if (name == "bm_block")
    {
    }
    else if (name == "tree")
    {
    }
    else if (name == "sb")
    {
    }
    else if (name == "file")
    {
    }
    else if (name == "ls")
    {
    }

    file = fopen((path + ".dot").c_str(), "w");
    fwrite(body.c_str(), body.length(), 1, file);
    fclose(file);

    string command_genere = "dot "+ path + ".dot -Tpng -o " + path + ".png";

    if (system(command_genere.c_str()) != 0)
        throw Exception("could not genere graph");

    return return_value;
}
