#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <cmath>

namespace { // anonymous namespace keeps these local

using std::string;
using std::vector;
using std::stringstream;

// Round down to nearest integer
inline int64_t
round_down(double x)
{
    return static_cast<int64_t>(std::floor(x));
}

// Divide two integers with double result
template <typename X, typename Y>
static double
true_div(X x, Y y)
{
    return static_cast<double>(x) / static_cast<double>(y);
}

// Helper function to test a string for a given suffix
// http://stackoverflow.com/questions/20446201
inline bool
has_suffix(const string &str, const string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Helper functions to split strings
// http://stackoverflow.com/a/236803/1877086
inline void
split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

inline vector<string>
split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

} // end anonymous namespace
