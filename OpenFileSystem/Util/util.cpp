#include <iostream>
#include <boost/algorithm/string.hpp>

using namespace std;

string unquote(string word)
{
    if (boost::algorithm::ends_with(word, "\"") || boost::algorithm::ends_with(word, "'"))
    {
        word = word.substr(0, word.length() - 1);
    }
    if (boost::algorithm::starts_with(word, "\"") || boost::algorithm::starts_with(word, "'"))
    {
        word = word.substr(1, word.length() - 1);
    }

    return word;
}
