#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <boost/algorithm/string.hpp>

#include "reportmanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"
#include "../Util/util.h"

using namespace std;

string rep_mbr(string name, MBR mbr)
{
    string s = "";

    s += " digraph G {\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";
    s += +" Foo [label=<\n";
    s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    s += +"   <tr><td><i>Nombre</i></td><td><i>Valor</i></td></tr>\n";

    s += "<tr><td align=\"left\">Nombre</td><td>" + name + "</td></tr>\n";
    s += "<tr><td align=\"left\">Tamaño</td><td>" + to_string(mbr.size) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fecha de creación</td><td>" + string(mbr.date).substr(0, 16) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Signature</td><td>" + to_string(mbr.signature) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fit</td><td>" + string(mbr.fit) + "</td></tr>\n";
    s += "<tr><td colspan='2'></td></tr>\n";

    for (int i = 0; i < 4; i++)
    {
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Nombre</td><td>" + string(mbr.partitions[i].name).substr(0, 16) + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Estado</td><td>" + mbr.partitions[i].status + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Tipo</td><td>" + mbr.partitions[i].type + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Fit</td><td>" + string(mbr.partitions[i].fit).substr(0, 2) + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Inicio</td><td>" + to_string(mbr.partitions[i].start) + "</td></tr>\n";
        s += "<tr><td align=\"left\">P" + to_string(i + 1) + ": Tamaño</td><td>" + to_string(mbr.partitions[i].size) + "</td></tr>\n";
    }

    s += "</table>>];";
    s += "} ";

    return s;
}

string rep_disk(string name, MBR mbr)
{
    string s = "";

    s += " digraph G {\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";
    s += +" Foo [label=<\n";
    s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    s += "<tr>\n";
    s += "  <td><br/>MBR<br/></td>\n";

    vector<int> starts;
    for (int i = 0; i < 4; i++)
        if (mbr.partitions[i].status != '\\')
            starts.push_back((mbr.partitions[i].start));

    sort(starts.begin(), starts.end());

    vector<vector<int>> occuppied;
    for (int start : starts)
        for (int i = 0; i < 4; i++)
            if (mbr.partitions[i].start == start)
            {
                vector<int> temp;
                temp.push_back(i);
                temp.push_back(start);
                temp.push_back(mbr.partitions[i].size);

                occuppied.push_back(temp);
            }

    vector<vector<int>> blank;
    if (occuppied.empty())
    {
        vector<int> temp;
        temp.push_back(sizeof(MBR));
        temp.push_back(mbr.size);
        blank.push_back(temp);
        s += "  <td>Libre<br/>100% del disco</td>\n";
    }
    else
    {
        if (sizeof(MBR) < occuppied.at(0).at(1))
        {
            vector<int> temp;
            temp.push_back(sizeof(MBR));
            temp.push_back(occuppied.at(0).at(1));
            blank.push_back(temp);
            s += "  <td>Libre<br/>" + to_string(occuppied.at(0).at(1) / mbr.size) + "% del disco</td>\n";
        }

        for (int i = 0; i < occuppied.size(); i++)
        {
            vector<int> temp;

            if (i != occuppied.size() - 1)
            {
                if (occuppied.at(i).at(1) + occuppied.at(i).at(2) < occuppied.at(i + 1).at(1))
                {
                    temp.push_back(occuppied.at(i).at(1) + occuppied.at(i).at(2));
                    temp.push_back(occuppied.at(i + 1).at(1));
                    blank.push_back(temp);

                    if (occuppied.at(i).at(0) == 'p')
                        s += "  <td>Primaria<br/>" + to_string(occuppied.at(i).at(2) / mbr.size) + "% del disco</td>\n";
                    else if (occuppied.at(i).at(0) == 'e')
                        s += "  <td>EXTENDIDA WIP<br/>" + to_string(occuppied.at(i).at(2) / mbr.size) + "% del disco</td>\n";

                    s += "  <td>Libre<br/>" + to_string((occuppied.at(i + 1).at(1) - (occuppied.at(i).at(1) + occuppied.at(i).at(2))) / mbr.size) + "% del disco</td>\n";
                }
            }
            else if (occuppied.at(i).at(2) < mbr.size)
            {
                temp.push_back(occuppied.at(i).at(1) + occuppied.at(i).at(2));
                temp.push_back(mbr.size);
                blank.push_back(temp);

                if (occuppied.at(i).at(0) == 'p')
                    s += "  <td>Primaria<br/>" + to_string(occuppied.at(i).at(2) / mbr.size) + "% del disco</td>\n";
                else if (occuppied.at(i).at(0) == 'e')
                    s += "  <td>EXTENDIDA WIP<br/>" + to_string(occuppied.at(i).at(2) / mbr.size) + "% del disco</td>\n";

                s += "  <td>Libre<br/>" + to_string((mbr.size - (occuppied.at(i).at(1) + occuppied.at(i).at(2))) / mbr.size) + "% del disco</td>\n";
            }
        }
    }

    s += "</tr>\n";
    s += "</table>>];";
    s += "} ";

    return s;
}

string rep_journaling(FILE *file, superblock super)
{
    if (super.filesystem_type != 3)
        throw Exception("partition format does not support journaling");

    string s = "";

    return s;
}

string rep_inode(FILE *file, superblock super)
{
    string s = "";

    s += " digraph G {\n";
    s += " rankdir=\"LR\"\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";

    fseek(file, super.inode_start, SEEK_SET);

    for (int i = 0; i < super.inodes_count; i++)
    {
        inode inodo;
        fread(&inodo, sizeof(inode), 1, file);

        if (string(inodo.ctime).empty())
            continue;

        if (i != 0)
            s += +" Foo_" + to_string(i - 1) + " -> Foo_" + to_string(i) + " ;\n";

        s += +" Foo_" + to_string(i) + " [label=<\n";
        s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
        s += +"   <tr><td colspan='2' bgcolor='#1ac6ff'><i>Inodo " + to_string(i) + "</i></td></tr>\n";
        s += "<tr><td align=\"left\">GID</td><td>" + to_string(inodo.uid) + "</td></tr>\n";
        s += "<tr><td align=\"left\">UID</td><td>" + to_string(inodo.gid) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Tamaño</td><td>" + to_string(inodo.size) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Fecha de creación</td><td>" + string(inodo.ctime).substr(0, 16) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Fecha de visualización</td><td>" + string(inodo.atime).substr(0, 16) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Fecha de modificación</td><td>" + string(inodo.mtime).substr(0, 16) + "</td></tr>\n";
        s += "<tr><td colspan='2'></td></tr>\n";

        for (int j = 0; j < 12; j++)
        {
            if (inodo.block[j] == -1)
                continue;
            s += "<tr><td align=\"left\">Bloque " + to_string(j) + "</td><td>" + to_string(inodo.block[j]) + "</td></tr>\n";
        }

        string t = "0";
        if (inodo.type == '\1')
            t = "1";

        s += "<tr><td colspan='2'></td></tr>\n";
        s += "<tr><td align=\"left\">Tipo</td><td>" + t + "</td></tr>\n";
        s += "<tr><td align=\"left\">Permisos</td><td>" + to_string(inodo.permissions) + "</td></tr>\n";

        s += "</table>>];\n";
    }

    s += "} ";

    return s;
}

string rep_block(FILE *file, superblock super)
{
    string s = "";

    s += " digraph G {\n";
    s += " rankdir=\"LR\"\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";

    for (int i = super.inode_start; i < super.block_start; i += super.inode_size)
    {
        inode inodo;
        fseek(file, i, SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);

        if (string(inodo.ctime).empty())
            continue;

        string last;
        bool first = true;

        if (inodo.type == '\0')
        {
            for (int j = 0; j < 12; j++)
            {
                if (inodo.block[j] == -1)
                    continue;

                folder_block block;
                fseek(file, super.block_start + inodo.block[j] * super.block_size, SEEK_SET);
                fread(&block, sizeof(folder_block), 1, file);

                if (!first)
                    s += +" Foo_" + last + " -> Foo_" + to_string(inodo.block[j]) + " ;\n";

                first = false;
                last = to_string(inodo.block[j]);

                s += +" Foo_" + last + " [label=<\n";
                s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
                s += +"   <tr><td colspan='2' bgcolor='#70db70'><i>Bloque Carpeta " + to_string(inodo.block[j]) + "</i></td></tr>\n";
                s += "<tr><td align=\"left\"><B>Nombre</B></td><td><B>Inodo</B></td></tr>\n";

                for (int k = 0; k < 4; k++)
                {
                    s += "<tr><td align=\"left\">" + string(block.content[k].name).substr(0, 12) + "</td><td>" + to_string(block.content[k].inode) + "</td></tr>\n";
                }

                s += "</table>>];\n";
            }
        }
        else if (inodo.type == '\1')
        {
            for (int j = 0; j < 12; j++)
            {
                if (inodo.block[j] == -1)
                    continue;

                file_block block;
                fseek(file, super.block_start + inodo.block[j] * super.block_size, SEEK_SET);
                fread(&block, sizeof(file_block), 1, file);

                if (!first)
                    s += +" Foo_" + last + " -> Foo_" + to_string(inodo.block[j]) + " ;\n";

                first = false;
                last = to_string(inodo.block[j]);

                s += +" Foo_" + to_string(inodo.block[j]) + " [label=<\n";
                s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
                s += +"   <tr><td colspan='2' bgcolor='#ebeb14'><i>Bloque Archivo " + to_string(inodo.block[j]) + "</i></td></tr>\n";

                s += "<tr><td align=\"left\">" + string(block.content).substr(0, 64) + "</td></tr>\n";

                s += "</table>>];\n";
            }
        }
    }

    s += "} ";

    return s;
}

string rep_bm_inode(FILE *file, superblock super)
{
    string s = "";

    int new_line = 0;
    char b;

    fseek(file, super.bm_inode_start, SEEK_SET);

    for (int i = 0; i < super.inodes_count; i++)
    {
        fread(&b, 1, 1, file);

        if (b == '\0')
            s += "0  ";
        else
            s += "1  ";

        new_line += 1;

        if (new_line == 20)
        {
            s += "\n";
            new_line = 0;
        }
    }

    return s;
}

string rep_bm_block(FILE *file, superblock super)
{
    string s = "";

    int new_line = 0;
    char b;

    fseek(file, super.bm_block_start, SEEK_SET);

    for (int i = 0; i < super.blocks_count; i++)
    {
        fread(&b, 1, 1, file);

        if (b == '\0')
            s += "0  ";
        else
            s += "1  ";

        new_line += 1;

        if (new_line == 20)
        {
            s += "\n";
            new_line = 0;
        }
    }

    return s;
}

string print_nodes(FILE *file, superblock super, int num_nodo)
{
    string s = "";

    fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
    inode inodo;
    fread(&inodo, sizeof(inode), 1, file);

    s += +"Node_" + to_string(num_nodo) + " [label=<\n";
    s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    s += +"   <tr><td port='t' colspan='2' bgcolor='#1ac6ff'><i>Inodo " + to_string(num_nodo) + "</i></td></tr>\n";
    s += "<tr><td align=\"left\">GID</td><td>" + to_string(inodo.uid) + "</td></tr>\n";
    s += "<tr><td align=\"left\">UID</td><td>" + to_string(inodo.gid) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Tamaño</td><td>" + to_string(inodo.size) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fecha de creación</td><td>" + string(inodo.ctime).substr(0, 16) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fecha de visualización</td><td>" + string(inodo.atime).substr(0, 16) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fecha de modificación</td><td>" + string(inodo.mtime).substr(0, 16) + "</td></tr>\n";
    s += "<tr><td colspan='2'></td></tr>\n";

    for (int i = 0; i < 12; i++)
        s += "<tr><td align=\"left\">Bloque " + to_string(i) + "</td><td port='" + to_string(i) + "'>" + to_string(inodo.block[i]) + "</td></tr>\n";

    string t = "0";
    if (inodo.type == '\1')
        t = "1";

    s += "<tr><td colspan='2'></td></tr>\n";
    s += "<tr><td align=\"left\">Tipo</td><td>" + t + "</td></tr>\n";
    s += "<tr><td align=\"left\">Permisos</td><td>" + to_string(inodo.permissions) + "</td></tr>\n";

    s += "</table>>];\n";

    if (inodo.type == 0)
    {
        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            folder_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            s += "Node_" + to_string(num_nodo) + ":" + to_string(i) + " -> Block_" + to_string(inodo.block[i]) + ";\n";

            s += +"Block_" + to_string(inodo.block[i]) + " [label=<\n";
            s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
            s += +"   <tr><td port='t' colspan='2' bgcolor='#70db70'><i>Bloque Carpeta " + to_string(inodo.block[i]) + "</i></td></tr>\n";
            s += "<tr><td align=\"left\"><B>Nombre</B></td><td><B>Inodo</B></td></tr>\n";

            for (int j = 0; j < 4; j++)
                s += "<tr><td align=\"left\">" + string(block.content[j].name).substr(0, 12) + "</td><td port='" + to_string(j) + "'>" + to_string(block.content[j].inode) + "</td></tr>\n";

            s += "</table>>];\n";

            for (int j = 0; j < 4; j++)
            {
                if (string(block.content[j].name) == "." || string(block.content[j].name) == ".." || block.content[j].inode == -1)
                    continue;

                s += "Block_" + to_string(inodo.block[i]) + ":" + to_string(j) + " -> Node_" + to_string(block.content[j].inode) + ";\n";
                s += print_nodes(file, super, block.content[j].inode);
            }
        }

        //bloques indirectos
    }
    else
    {
        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            file_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
            fread(&block, sizeof(file_block), 1, file);

            s += "Node_" + to_string(num_nodo) + ":" + to_string(i) + " -> Block_" + to_string(inodo.block[i]) + ";\n";

            s += +"Block_" + to_string(inodo.block[i]) + " [label=<\n";
            s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
            s += +"   <tr><td port='t' colspan='2' bgcolor='#ebeb14'><i>Bloque Archivo " + to_string(inodo.block[i]) + "</i></td></tr>\n";
            s += "<tr><td align=\"left\">" + string(block.content).substr(0, 64) + "</td></tr>\n";
            s += "</table>>];\n";
        }

        //bloques indirectos
    }

    return s;
}

string rep_tree(FILE *file, superblock super)
{
    string s = "";

    s += " digraph G {\n";
    s += " rankdir=\"LR\"\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";
    s += print_nodes(file, super, 0);
    s += "}";

    return s;
}

string rep_sb(superblock super)
{
    string s = "";

    s += " digraph G {\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";
    s += +" Foo [label=<\n";
    s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    s += +"   <tr><td><i>Nombre</i></td><td><i>Valor</i></td></tr>\n";

    s += "<tr><td align=\"left\">Sistema de archivos</td><td>" + to_string(super.filesystem_type) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Montajes</td><td>" + to_string(super.mnt_count) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Número mágico</td><td>" + to_string(super.magic) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fecha de visualización</td><td>" + string(super.umtime).substr(0, 16) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Fecha de modificación</td><td>" + string(super.mtime).substr(0, 16) + "</td></tr>\n";
    s += "<tr><td colspan='2'></td></tr>\n";

    s += "<tr><td align=\"left\">Inicio de bitmap de inodos</td><td>" + to_string(super.bm_inode_start) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Inicio de bitmap de bloques</td><td>" + to_string(super.bm_block_start) + "</td></tr>\n";
    s += "<tr><td colspan='2'></td></tr>\n";

    s += "<tr><td align=\"left\">Número de inodos</td><td>" + to_string(super.inodes_count) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Número de inodos libres</td><td>" + to_string(super.free_inodes_count) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Tamaño de inodo</td><td>" + to_string(super.inode_size) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Inicio de inodos</td><td>" + to_string(super.inode_start) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Primer inodo libre</td><td>" + to_string(super.first_inode) + "</td></tr>\n";
    s += "<tr><td colspan='2'></td></tr>\n";

    s += "<tr><td align=\"left\">Número de bloques</td><td>" + to_string(super.blocks_count) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Número de bloques libres</td><td>" + to_string(super.free_blocks_count) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Tamaño de bloque</td><td>" + to_string(super.block_size) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Inicio de bloques</td><td>" + to_string(super.block_start) + "</td></tr>\n";
    s += "<tr><td align=\"left\">Primer bloque libre</td><td>" + to_string(super.first_block) + "</td></tr>\n";

    if (super.filesystem_type == 3)
    {
        s += "<tr><td colspan='2'></td></tr>\n";
        s += "<tr><td align=\"left\">Número de journal</td><td>" + to_string(super.journal_count) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Número de journal libres</td><td>" + to_string(super.free_journal_count) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Tamaño de journal</td><td>" + to_string(super.journal_size) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Inicio de journaling</td><td>" + to_string(super.journal_start) + "</td></tr>\n";
        s += "<tr><td align=\"left\">Primer journal libre</td><td>" + to_string(super.first_journal) + "</td></tr>\n";
    }

    s += "</table>>];";
    s += "} ";

    return s;
}

string rep_file(FILE *file, superblock super, string ruta)
{
    if (ruta.empty())
        throw Exception("-ruta parameter missing for -name=file");

    stringstream ss(ruta.substr(1, ruta.length() - 1));

    string dir;
    vector<string> words;
    while (getline(ss, dir, '/'))
    {
        words.push_back(dir);
    }
    string file_name = words.at(words.size() - 1);

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    int num_nodo = 0;

    for (string dir : words)
    {
        bool found = false;

        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            folder_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
            {
                if (block.content[j].name == dir)
                {
                    num_nodo = block.content[j].inode;
                    found = true;
                }
            }

            if (found)
                break;
        }

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (inodo.type != '\1')
        throw Exception("not a file");

    string content = "";
    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        content += string(block.content).substr(0, 64);
    }

    size_t pos = 0;
    while ((pos = content.find("\n", pos)) != std::string::npos)
    {
        content.replace(pos, 1, "<br/>");
        pos += 5;
    }

    string s = "";

    s += " digraph G {\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";

    s += +"FILE [label=<\n";
    s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    s += +"   <tr><td bgcolor='#ebeb14'><i>" + file_name + "</i></td></tr>\n";
    s += "<tr><td align=\"left\">" + content + "</td></tr>\n";
    s += "</table>>];\n";
    s += "}";

    return s;
}

string rep_ls(FILE *file, superblock super, string ruta)
{
    if (ruta.empty())
        throw Exception("-ruta parameter missing for -name=file");

    stringstream ss(ruta.substr(1, ruta.length() - 1));

    string dir;
    vector<string> words;
    while (getline(ss, dir, '/'))
    {
        words.push_back(dir);
    }

    inode inodo;
    fseek(file, super.inode_start, SEEK_SET);
    fread(&inodo, sizeof(inode), 1, file);

    int num_nodo = 0;

    for (string dir : words)
    {
        bool found = false;

        for (int i = 0; i < 12; i++)
        {
            if (inodo.block[i] == -1)
                continue;

            folder_block block;
            fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
            fread(&block, sizeof(folder_block), 1, file);

            for (int j = 0; j < 4; j++)
            {
                if (block.content[j].name == dir)
                {
                    num_nodo = block.content[j].inode;
                    found = true;
                }
            }

            if (found)
                break;
        }

        if (!found)
        {
            string msg = "'" + dir + "' not found";
            throw Exception(msg.c_str());
        }

        fseek(file, super.inode_start + num_nodo * sizeof(inode), SEEK_SET);
        fread(&inodo, sizeof(inode), 1, file);
    }

    if (inodo.type != '\0')
        throw Exception("not a folder");

    string s = "";

    s += " digraph G {\n";
    s += " rankdir=\"LR\"\n";
    s += +" node [label=\"\\N\", fontsize=13 shape=plaintext fontname = \"helvetica\"];\n";

    s += +"LS [label=<\n";
    s += +"<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    //s += +"   <tr><td port='t' colspan='2' bgcolor='#666699'><i>" + dir_name + "</i></td></tr>\n";
    s += "<tr><td>Permisos</td><td>Usuario</td><td>Grupo</td><td>Tamaño</td><td>Fecha de creación</td><td>Fecha de modificación</td><td>Hora de modificación</td><td>Tipo</td><td>Nombre</td></tr>\n";

    string users_content = "";
    inode inodo_users;
    fseek(file, super.inode_start + sizeof(inode), SEEK_SET);
    fread(&inodo_users, sizeof(inode), 1, file);

    for (int i = 0; i < 12; i++)
    {
        if (inodo_users.block[i] == -1)
            continue;

        file_block block;
        fseek(file, super.block_start + inodo_users.block[i] * sizeof(file_block), SEEK_SET);
        fread(&block, sizeof(file_block), 1, file);

        users_content += block.content;
    }

    stringstream su(users_content);
    string line;
    vector<vector<string>> users, groups;

    while (getline(su, line, '\n'))
    {
        stringstream us(line);
        string substr;
        vector<string> words;

        while (getline(us, substr, ','))
            words.push_back(substr);

        if (words.at(1) == "G")
            groups.push_back(words);
        else
            users.push_back(words);
    }

    for (int i = 0; i < 12; i++)
    {
        if (inodo.block[i] == -1)
            continue;

        folder_block block;
        fseek(file, super.block_start + inodo.block[i] * sizeof(folder_block), SEEK_SET);
        fread(&block, sizeof(folder_block), 1, file);

        for (int j = 0; j < 4; j++)
        {
            if (block.content[j].inode == -1)
                continue;

            inode inodo_temp;
            fseek(file, super.inode_start + block.content[j].inode * sizeof(inode), SEEK_SET);
            fread(&inodo_temp, sizeof(inode), 1, file);

            string permissions = "-", user, group, type;

            for (int k = 0; k < 3; k++)
            {
                int permission = stoi(to_string(inodo_temp.permissions).substr(k, k + 1));

                int x = permission % 2;
                int w = (permission / 2) % 2;
                int r = (permission / 4) % 2;

                if (r == 1)
                    permissions += "r";
                else
                    permissions += "-";

                if (w == 1)
                    permissions += "w";
                else
                    permissions += "-";

                if (x == 1)
                    permissions += "x";
                else
                    permissions += "-";
            }

            for (vector<string> u : users)
                if (stoi(u.at(0)) == inodo_temp.uid)
                    user = u.at(3);

            for (vector<string> g : groups)
                if (stoi(g.at(0)) == inodo_temp.gid)
                    group = g.at(2);

            if (inodo_temp.type == '\0')
                type = "Carpeta";
            else
                type = "Archivo";

            s += "<tr><td>" + permissions + "</td><td>" + user + "</td><td>" + group + "</td><td>" + to_string(inodo_temp.size) + "</td><td>" + string(inodo_temp.ctime).substr(0, 16) + "</td><td>" + string(inodo_temp.mtime).substr(0, 10) + "</td><td>" + string(inodo_temp.mtime).substr(11, 5) + "</td><td>" + type + "</td><td>" + string(block.content[j].name) + "</td></tr>\n";
        }
    }

    s += "</table>>];\n";
    s += "}";

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
    if (id.length() == 4)
        data = getPartitionMountedByID(id.substr(3, 4)[0], stoi(id.substr(2, 3)));
    else
        data = getPartitionMountedByID(id.substr(4, 5)[0], stoi(id.substr(2, 4)));

    if (data.empty())
        throw Exception("id does not exist");

    const char *disk_path = data[1];
    string partition_name = data[0];
    string disk_name = string(disk_path);

    const size_t last_slash_idx = disk_name.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
        disk_name.erase(0, last_slash_idx + 1);

    FILE *file = fopen(disk_path, "rb");

    fseek(file, 0, SEEK_SET);
    MBR mbr;
    fread(&mbr, sizeof(MBR), 1, file);

    Partition *partition = getPartition(file, &mbr, partition_name);

    if (name != "mbr" && name != "disk" && partition->status != '1')
        throw Exception("partition is not formated");

    fseek(file, partition->start, SEEK_SET);
    superblock super;
    fread(&super, sizeof(superblock), 1, file);

    string body;

    if (name == "mbr")
    {
        body = rep_mbr(disk_name, mbr);
    }
    else if (name == "disk")
    {
        body = rep_disk(disk_name, mbr);
    }
    else if (name == "journaling")
    {
        body = rep_journaling(file, super);
    }
    else if (name == "inode")
    {
        body = rep_inode(file, super);
    }
    else if (name == "block")
    {
        body = rep_block(file, super);
    }
    else if (name == "bm_inode")
    {
        body = rep_bm_inode(file, super);
    }
    else if (name == "bm_block")
    {
        body = rep_bm_block(file, super);
    }
    else if (name == "tree")
    {
        body = rep_tree(file, super);
    }
    else if (name == "sb")
    {
        body = rep_sb(super);
    }
    else if (name == "file")
    {
        body = rep_file(file, super, ruta);
    }
    else if (name == "ls")
    {
        body = rep_ls(file, super, ruta);
    }

    if (name != "bm_inode" && name != "bm_block")
    {
        file = fopen((path + ".dot").c_str(), "w");
        fwrite(body.c_str(), body.length(), 1, file);
        fclose(file);

        string command_genere = "dot " + path + ".dot -Tpng -o " + path + ".png";

        if (system(command_genere.c_str()) != 0)
            throw Exception("could not genere report");
    }
    else
    {
        file = fopen((path + ".bm").c_str(), "w");
        fwrite(body.c_str(), body.length(), 1, file);
        fclose(file);
    }

    return 0;
}
