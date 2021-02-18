#include <iostream>
#include <fstream>
#include <cstdlib>

#include "../Analyzer/scriptreader.h"
#include "../Util/util.h"

using namespace std;

int exec(string path)
{
    ifstream data(path);
    string line;

    if (data)
    {
        while (!data.eof())
            while (getline(data, line))
                cout << "$ " << line << endl << "return value: " << readline(line) << endl;

        data.close();
        cout << "  >> file closed <<" << endl;

        return 0;
    }
    else
    {
        throw Exception("file not found");
    }
}
