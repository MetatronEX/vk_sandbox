#ifndef COMMON_HPP
#define COMMON_HPP

#include <fstream>
#include <optional>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <array>
#include <set>
#include <unordered_map>
#include <numeric>
#include <limits>
#include <ctime>
#include <chrono>
#include <random>
#include <algorithm>

bool file_exists(const char* filename)
{
    std::ifstream f(filename);
    return !f.fail();
}

uint32_t align(const uint32_t value, const uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

#endif