#include <iostream>
#include <string>
// #include <boost/algorithm/string.hpp>

#include "Analyzer/scriptreader.h"

using namespace std;

int main()
{
    string command;

    while (true)
    {
        try
        {
            cout << "\033[1;33m$\033[0m ";
            getline(cin, command);

            command = "exec -path='test.script'"; // quemado

            // if (boost::algorithm::starts_with(command, "#") || std::all_of(command.begin(), command.end(), [](char c) { return std::isspace(c); }))
            //     continue; // COMENTARIOS

            int return_value = readline(command);
            if (return_value == 0)
                cout << "\033[1;32m" << return_value << "\033[0m" << endl;
        }
        catch (const std::exception &e)
        {
            cout << "\033[1;41m[##]\033[0m \033[1;31m" << e.what() << "\033[0m" << endl;
        }
    }
}
