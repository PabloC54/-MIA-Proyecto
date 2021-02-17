#ifndef UTIL_H
#define UTIL_H

#include <boost/algorithm/string.hpp>
using namespace std;

class Exception : public std::runtime_error
{
public:
    Exception(const char *what) : runtime_error(what) {}
};

string unquote(string);

#endif