#pragma once

#include <string>
#include <cstdint>

class Hooks
{
public:
    // Singleton getter
    static Hooks& getInstance();
    // Marks the start of a new phase of computation
    void region_begin(std::string name);
    // Marks the end of the current phase of computation
    void region_end();
    // Set a custom data value that will be included in the JSON output at the end of every region
    void set_attr(std::string key, uint64_t value);
    void set_attr(std::string key, int64_t value);
    void set_attr(std::string key, double value);
    void set_attr(std::string key, std::string value);
    // Set a custom data value that will be included in the JSON output at the end of the next region
    void set_stat(std::string key, uint64_t value);
    void set_stat(std::string key, int64_t value);
    void set_stat(std::string key, double value);
    void set_stat(std::string key, std::string value);
private:
    Hooks();
    ~Hooks();
    Hooks(Hooks const&);
    Hooks& operator=(Hooks const&);
    class impl;
    impl * pimpl;
};
