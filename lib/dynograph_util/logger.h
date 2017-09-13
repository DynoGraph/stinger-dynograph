#pragma once
#include "mpi_macros.h"
#include <sstream>
#include <iostream>
#include <string>

namespace DynoGraph {

class Logger
{
protected:
    const std::string msg = "[DynoGraph] ";
    std::ostream &out;
    std::ostringstream oss;
    Logger (std::ostream &out) : out(out) {}
public:
    // Print to error stream with prefix
    template <class T>
    Logger& operator<<(T&& x) {
        MPI_RANK_0_ONLY {
        oss << std::forward<T>(x);
        // Once we have a full line, print with prefix
        if (oss.str().back() == '\n') {
            out << msg << oss.str();
            oss.str("");
        }
        }
        return *this;
    }
    // Handle IO manipulators
    Logger& operator<<(std::ostream& (*manip)(std::ostream&))
    {
        oss << manip;
        return *this;
    }
    // Singleton getter
    static Logger& get_instance()
    {
        static Logger instance(std::cerr);
        return instance;
    }
    // Destructor
    virtual ~Logger()
    {
        // Flush remaining buffered output in case of forgotten newline
        if (oss.str().size() > 0) { out << msg << oss.str(); }
    }
};

// Terminate the benchmark in the event of an error
inline void
die()
{
    exit(-1);
}

} // end namespace DynoGraph
