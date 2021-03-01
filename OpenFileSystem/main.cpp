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
            cout << "$ ";
            getline(cin, command);

            command = "exec -path='test.script'"; // quemado

            // if (boost::algorithm::starts_with(command, "#") || std::all_of(command.begin(), command.end(), [](char c) { return std::isspace(c); }))
            //     continue; // COMENTARIOS

            cout << readline(command) << endl;
        }
        catch (const std::exception &ex)
        {
            cout << "[###] Fatal" << endl;
            cout << ex.what() << endl;
        }
    }
}
