// Test driver for static_vector

#include "SelfCount.hpp"
#include <iostream>
#include <vector>
#include <list>

int main() {
    std::vector<int> src {4, 5, 6, 7, 8};
    std::vector<SelfCount> tgt1(src.begin(), src.end());
    tgt1.assign(src.begin(), src.end());
    tgt1.push_back(8);
    std::vector<SelfCount> tgt2 {1,14,91,22};
}
