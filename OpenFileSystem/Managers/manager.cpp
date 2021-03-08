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
        cout << ">> file opened <<" << endl;
        while (!data.eof())
            while (getline(data, line))
            {
                if (boost::algorithm::starts_with(line, "#") || std::all_of(line.begin(), line.end(), [](char c) { return std::isspace(c); }))
                    continue; // COMENTARIOS

                cout << "$ " << line << ": ";
                cout << readline(line) << endl;
            }

        data.close();
        cout << ">> file closed <<" << endl;

        return 0;
    }
    else
    {
        throw Exception("file not found");
    }
}
