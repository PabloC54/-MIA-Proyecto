#include <iostream>
#include <string>

#include "Analyzer/scriptreader.h"

using namespace std;

int main()
{
    string command;

    while (true)
    {
        cout << "$ ";
        getline(cin, command);

        // command = "exec -path='test.script'";
        if (command == "exit")
        {
            cout << " exiting . . ." << endl;
            break;
        }
        else if (command == "pause")
        {
            cout << "Press enter to continue . . ." << endl;
            cin.ignore();
            continue;
        }

        cout << readline(command) << endl;
    }
}
