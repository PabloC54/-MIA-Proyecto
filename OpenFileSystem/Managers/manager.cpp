#include <iostream>
#include <fstream>
#include <cstdlib>
#include <boost/algorithm/string.hpp>

#include "usermanager.h"
#include "../Structs/diskstructs.h"
#include "../Structs/mountstructs.h"
#include "../Structs/partitionstructs.h"
#include "../Analyzer/scriptreader.h"
#include "../Util/util.h"

using namespace std;

int exec(string path)
{
    ifstream data(path);
    string line;

    if (data)
    {
        cout << "\033[1;36m>> file opened <<\033[0m" << endl;
        while (!data.eof())
            while (getline(data, line))
            {
                if (boost::algorithm::starts_with(line, "#") || std::all_of(line.begin(), line.end(), [](char c) { return std::isspace(c); }))
                {
                    cout << "\033[1;36m" << line << "\033[0m" << endl;
                    continue;
                }

                cout << "\033[1;33m$\033[0m " << line << " : ";

                int return_value = readline(line);
                if (return_value == 0)
                    cout << "\033[1;32m" << return_value << "\033[0m" << endl;
            }

        data.close();
        cout << "\033[1;36m>> file closed <<\033[0m" << endl;

        return 0;
    }
    else
    {
        throw Exception("file not found");
    }
}

int recovery(string id)
{
    vector<const char *> data;
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

    Partition *partition = getPartition(file, &mbr, partition_name);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    if (super.filesystem_type != 3)
        throw Exception("partition does not support recovery");

    // recuperando usando journaling

    fseek(file, super.journal_start, SEEK_SET);

    journal j;
    fread(&j, sizeof(journal), 1, file);

    while (j.type != '\\')
    {

        string operation = string(j.operation);
        string path = string(j.path);

        string cont, size;
        if (!string(j.content).empty())
            cont = string(j.content);
        if (!to_string(j.size).empty())
            size = string(j.content);

        string command = operation + " -path=\"" + path + "\" -cont=\"" + cont + "\" -size=\"" + size + "\"";

        cout << "\033[1;33m$\033[0m " << command << " : ";
        
        int return_value = readline(command);
        if (return_value == 0)
            cout << "\033[1;32m" << return_value << "\033[0m" << endl;

        journal j;
        fread(&j, sizeof(journal), 1, file);
    }

    return 0;
}

int loss(string id)
{
    vector<const char *> data;
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

    Partition *partition = getPartition(file, &mbr, partition_name);

    superblock super;
    fseek(file, partition->start, SEEK_SET);
    fread(&super, sizeof(superblock), 1, file);

    if (super.filesystem_type != 3)
    {
        throw Exception("partition does not support recovery, proceed? (y/n)");

        string rep;
        cin >> rep;

        if (rep == "n" || rep == "N")
            return 0;
    }

    // rellenando de 0s

    fseek(file, super.bm_inode_start, SEEK_SET);
    for (int i = 0; i < super.inodes_count; i++)
        fwrite("\0", 1, 1, file);

    fseek(file, super.bm_block_start, SEEK_SET);
    for (int i = 0; i < super.blocks_count; i++)
        fwrite("\0", 1, 1, file);

    fseek(file, super.inode_start, SEEK_SET);
    for (int i = 0; i < super.inodes_count * super.inode_size; i++)
        fwrite("\0", 1, 1, file);

    fseek(file, super.block_start, SEEK_SET);
    for (int i = 0; i < super.blocks_count * super.block_size; i++)
        fwrite("\0", 1, 1, file);

    return 0;
}
