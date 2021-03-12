#include <iostream>
#include <fstream>
#include <cstdlib>
#include <boost/algorithm/string.hpp>

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

    return 0;
}

int loss(string id)
{

    return 0;
}
