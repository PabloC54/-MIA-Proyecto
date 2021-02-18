#ifndef Util
#define Util

#include <boost/algorithm/string.hpp>
using namespace std;

string unquote(string);

class Exception : public std::runtime_error
{
public:
    Exception(const char *what) : runtime_error(what) {}
};

#endif