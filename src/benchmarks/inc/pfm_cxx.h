#ifndef _PFM_CXX_H
#define _PFM_CXX_H

// Adapted from GraphBIG <https://github.com/graphbig/graphBIG>

#include <string>

class pfm_cxx
{
public:
    static pfm_cxx& getInstance();
    bool event_encoding(const std::string& event, unsigned int& type, unsigned long long& config);
    pfm_cxx(pfm_cxx const&)         = delete;
    void operator=(pfm_cxx const&)  = delete;
private:
    pfm_cxx();
    ~pfm_cxx();
};
#endif
