#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include <regex>
#include <boost/algorithm/string.hpp>

#include "diskmanager.h"
#include "usermanager.h"
#include "storagemanager.h"
#include "reportmanager.h"
#include "util.h"

using namespace std;

int readline(string line);
int exec(string path);

int main()
{
    string command;

    while (true)
    {
        cout << "$ ";
        getline(cin, command);
        command = "exec -path='test.script'";
        if (command != "exit")
        {
            int val = readline(command);
            cout << val << endl;
            continue;
        }

        break;
    }

    cout << " exiting . . ." << endl;
}

int readline(string line)
{
    if (boost::algorithm::starts_with(line, "#"))
        return 0; // COMENTARIOS

    transform(line.begin(), line.end(), line.begin(), ::tolower);
    vector<string> words;
    istringstream ss(line);

    string word;
    regex re("^-.+=[\"']");

    while (ss >> word)
    {
        if (boost::algorithm::starts_with(word, "#"))
            break; // COMENTARIOS

        if (!regex_search(word, re))
            words.push_back(word);
        else
        {
            string temp;
            while (true)
            {
                if (boost::algorithm::ends_with(word, "\"") || boost::algorithm::ends_with(word, "'"))
                    break;

                ss >> temp;
                word += " " + temp;
            }

            words.push_back(word);
        }
    }

    try
    {
        string command = words[0];
        map<string, string> params;
        string string_temp;

        for (int i = 1; i < words.size(); i++) // extrayendo los parametros en un map
        {
            stringstream ss(words[i]);
            vector<string> vector_temp;

            while (std::getline(ss, string_temp, '='))
                vector_temp.push_back(string_temp);

            params[vector_temp[0]] = unquote(vector_temp[1]);
        }

        map<string, string>::iterator it;

        // print map
        // for (it = params.begin(); it != params.end(); ++it)
        // {
        //     cout << it->first << " => " << it->second << '\n';
        // }

        if (command == "mkdisk")
        { // DISK MANAGER
            string size, f, u, path;

            if (params.find("-size") != params.end())
                size = params["-size"];
            if (params.find("-f") != params.end())
                f = params["-f"];
            if (params.find("-u") != params.end())
                u = params["-u"];
            if (params.find("-path") != params.end())
                path = params["-path"];

            if (size.empty())
                throw Exception("-size parameter missing");
            if (path.empty())
                throw Exception("-path parameter missing");

            return mkdisk(size, f, u, path);
        }
        else if (command == "rmdisk")
        {
            string path;

            if (params.find("-path") != params.end())
                path = params["-path"];

            if (path.empty())
                throw Exception("-path parameter missing");

            return rmdisk(path);
        }
        else if (command == "fdisk")
        {
            string size, u, path, type, f, _delete, name, add;

            if (params.find("-size") != params.end())
                size = params["-size"];
            if (params.find("-u") != params.end())
                u = params["-u"];
            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-type") != params.end())
                type = params["-type"];
            if (params.find("-f") != params.end())
                f = params["-f"];
            if (params.find("-delete") != params.end())
                _delete = params["-delete"];
            if (params.find("-name") != params.end())
                name = params["-name"];
            if (params.find("-add") != params.end())
                add = params["-add"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (type.empty())
                throw Exception("-type parameter missing");
            if (name.empty())
                throw Exception("-name parameter missing");

            return fdisk(size, u, path, type, f, _delete, name, add);
        }
        else if (command == "mount")
        {
            string path, name;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-name") != params.end())
                name = params["-name"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (name.empty())
                throw Exception("-name parameter missing");

            return mount(path, name);
        }
        else if (command == "unmount")
        {
            string id;

            if (params.find("-id") != params.end())
                id = params["-id"];

            if (id.empty())
                throw Exception("-id parameter missing");

            return unmount(id);
        }
        else if (command == "mkfs")
        {
            string id, type, fs;

            if (params.find("-id") != params.end())
                id = params["-id"];
            if (params.find("-type") != params.end())
                type = params["-type"];
            if (params.find("-fs") != params.end())
                fs = params["-fs"];

            if (id.empty())
                throw Exception("-id parameter missing");

            return mkfs(id, type, fs);
        }
        else if (command == "login")
        { // USER MANAGER
            string usuario, password, id;

            if (params.find("-usuario") != params.end())
                usuario = params["-usuario"];
            if (params.find("-password") != params.end())
                password = params["-password"];
            if (params.find("-id") != params.end())
                id = params["-id"];

            if (usuario.empty())
                throw Exception("-usuario parameter missing");
            if (password.empty())
                throw Exception("-password parameter missing");
            if (id.empty())
                throw Exception("-id parameter missing");

            return login(usuario, password, id);
        }
        else if (command == "logout")
        {
            return logout();
        }
        else if (command == "mkgrp")
        {
            string name;

            if (params.find("-name") != params.end())
                name = params["-name"];

            if (name.empty())
                throw Exception("-name parameter missing");

            return mkgrp(name);
        }
        else if (command == "rmgrp")
        {
            string name;

            if (params.find("-name") != params.end())
                name = params["-name"];

            if (name.empty())
                throw Exception("-name parameter missing");

            return rmgrp(name);
        }
        else if (command == "mkusr")
        {
            string usr, pwd, grp;

            if (params.find("-usr") != params.end())
                usr = params["-usr"];
            if (params.find("-pwd") != params.end())
                pwd = params["-pwd"];
            if (params.find("-grp") != params.end())
                grp = params["-grp"];

            if (usr.empty())
                throw Exception("-usr parameter missing");
            if (pwd.empty())
                throw Exception("-pwd parameter missing");
            if (grp.empty())
                throw Exception("-grp parameter missing");

            return mkusr(usr, pwd, grp);
        }
        else if (command == "rmusr")
        {
            string usr;

            if (params.find("-usr") != params.end())
                usr = params["-usr"];

            if (usr.empty())
                throw Exception("-usr parameter missing");

            return rmusr(usr);
        }
        else if (command == "chmod")
        { // STORAGE MANAGER
            string path, ugo, r;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-ugo") != params.end())
                ugo = params["-ugo"];
            if (params.find("-r") != params.end())
                r = params["-r"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (ugo.empty())
                throw Exception("-ugo parameter missing");

            return chmod(path, ugo, r);
        }
        else if (command == "mkfile")
        {
            string path, r, size, cont;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-r") != params.end())
                r = params["-r"];
            if (params.find("-size") != params.end())
                size = params["-size"];
            if (params.find("-cont") != params.end())
                cont = params["-cont"];

            if (path.empty())
                throw Exception("-path parameter missing");

            return mkfile(path, r, size, cont);
        }
        else if (command == "cat")
        {
            string filen;

            if (params.find("-filen") != params.end())
                filen = params["-filen"];

            if (filen.empty())
                throw Exception("-filen parameter missing");

            return cat(filen);
        }
        else if (command == "rem")
        {
            string path;

            if (params.find("-path") != params.end())
                path = params["-path"];

            if (path.empty())
                throw Exception("-path parameter missing");

            return rem(path);
        }
        else if (command == "edit")
        {
            string path, cont;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-cont") != params.end())
                cont = params["-cont"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (cont.empty())
                throw Exception("-cont parameter missing");

            return edit(path, cont);
        }
        else if (command == "ren")
        {
            string path, name;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-name") != params.end())
                name = params["-name"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (name.empty())
                throw Exception("-name parameter missing");

            return ren(path, name);
        }
        else if (command == "mkdir")
        {
            string path, p;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-p") != params.end())
                p = params["-p"];

            if (path.empty())
                throw Exception("-path parameter missing");

            return mkdir(path, p);
        }
        else if (command == "cp")
        {
            string path, dest;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-dest") != params.end())
                dest = params["-dest"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (dest.empty())
                throw Exception("-dest parameter missing");

            return cp(path, dest);
        }
        else if (command == "mv")
        {
            string path, dest;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-dest") != params.end())
                dest = params["-dest"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (dest.empty())
                throw Exception("-dest parameter missing");

            return mv(path, dest);
        }
        else if (command == "find")
        {
            string path, name;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-name") != params.end())
                name = params["-name"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (name.empty())
                throw Exception("-name parameter missing");

            return find(path, name);
        }
        else if (command == "chown")
        {
            string path, r, usr;

            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-r") != params.end())
                r = params["-r"];
            if (params.find("-usr") != params.end())
                usr = params["-usr"];

            if (path.empty())
                throw Exception("-path parameter missing");
            if (usr.empty())
                throw Exception("-usr parameter missing");

            return chown(path, r, usr);
        }
        else if (command == "chgrp")
        {
            string usr, grp;

            if (params.find("-usr") != params.end())
                usr = params["-usr"];
            if (params.find("-grp") != params.end())
                grp = params["-grp"];

            if (usr.empty())
                throw Exception("-usr parameter missing");
            if (grp.empty())
                throw Exception("-grp parameter missing");

            return chgrp(usr, grp);
        }
        else if (command == "rep")
        { // REPORT MANAGER

            string name, path, id, ruta;

            if (params.find("-name") != params.end())
                name = params["-name"];
            if (params.find("-path") != params.end())
                path = params["-path"];
            if (params.find("-id") != params.end())
                id = params["-id"];
            if (params.find("-ruta") != params.end())
                ruta = params["-ruta"];

            if (name.empty())
                throw Exception("-name parameter missing");
            if (path.empty())
                throw Exception("-path parameter missing");
            if (id.empty())
                throw Exception("-id parameter missing");

            return rep(name, path, id, ruta);
        }
        else if (command == "pause")
        { // PAUSE
            cout << "Press enter to continue . . ." << endl;
            cin.ignore();
            return 0;
        }
        else if (command == "exec")
        { // READ SCRIPT

            string path;

            if (params.find("-path") != params.end())
                path = params["-path"];

            if (path.empty())
                throw Exception("-path parameter missing");

            return exec(path);
        }
        else
            throw Exception("command unrecognized");
    }
    catch (const Exception &e)
    {
        cout << "[#] Exception: " << e.what() << endl;
        return -1;
    }
}

int exec(string path)
{
    try
    {
        ifstream data(path);

        if (data)
        {
            while (!data.eof())
            {
                string line;

                while (getline(data, line))
                {
                    cout << "$ " << line + ": " << readline(line) << endl;
                }
            }

            data.close();
            cout << "  >> file closed <<" << endl;

            return 0;
        }
        else
        {
            throw Exception("file not found");
        }
    }
    catch (const Exception &e)
    {
        cout << "[#] Exception: " << e.what() << endl;
        return -1;
    }
}