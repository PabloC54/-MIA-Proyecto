#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "Disk/DiskManager.h"
#include "User/UserManager.h"
#include "Storage/StorageManager.h"
#include "Report/ReportManager.h"

using namespace std;

int main()
{
    string command = "";

    while (true)
    {

        cout << "$";
        cin >> command;

        trim(command);
        transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command != "exit")
        {
            int val = read(command);
            // cout << val << endl;
            continue;
        }

        break;
    }
}

int exec(string path)
{
    ifstream data(path);

    if (data)
    {
        while (!data.eof())
        {
            string line;

            while (getline(data, line))
            {
                cout << line + ": " << read(line) << endl;
            }
        }

        data.close();
        cout << "  >> file closed <<" << endl;

        return 0;
    }
    else
    {
        cerr << "No se encontró el archivo" << endl;
        return 1;
    }
}

int read(string line)
{
    istringstream str(line);

    string word;
    vector<string> words;

    while (str >> word)
    {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        words.push_back(word);
    }

    try
    {
        int val = 2;

        string cmd = words[0];
        if (cmd == "mkdisk")
        { // DISK MANAGER
        }
        else if (cmd == "rmdisk")
        {
        }
        else if (cmd == "fdisk")
        {
        }
        else if (cmd == "mount")
        {
        }
        else if (cmd == "unmount")
        {
        }
        else if (cmd == "login")
        { // USER MANAGER
        }
        else if (cmd == "logout")
        {
        }
        else if (cmd == "mkgrp")
        {
        }
        else if (cmd == "rmgrp")
        {
        }
        else if (cmd == "mkusr")
        {
        }
        else if (cmd == "rmusr")
        {
        }
        else if (cmd == "chmod")
        { // STORAGE MANAGER
        }
        else if (cmd == "mkfile")
        {
        }
        else if (cmd == "cat")
        {
        }
        else if (cmd == "rem")
        {
        }
        else if (cmd == "edit")
        {
        }
        else if (cmd == "ren")
        {
        }
        else if (cmd == "mkdir")
        {
        }
        else if (cmd == "cp")
        {
        }
        else if (cmd == "mv")
        {
        }
        else if (cmd == "find")
        {
        }
        else if (cmd == "chown")
        {
        }
        else if (cmd == "chgrp")
        {
        }
        else if (cmd == "rep")
        { // REPORT MANAGER
        }
        else if (cmd == "exec")
        { // READ SCRIPT
            val = exec(words[1]);
        }
        else
        {
            cout << words[0] + " no reconocido" << endl;
            val = -1;
        }

        cout << val << endl;
    }
    catch (const std::out_of_range &ex)
    {
        std::cout << "out_of_range Exception Caught :: " << ex.what() << std::endl;
    }

    return 0;
}
