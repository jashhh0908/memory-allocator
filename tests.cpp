#include "allocator.h"
#include <iostream>

int main() {
    Block block;
    char memory[1024];

    block.ptr = memory;
    block.capacity = 1024;
    block.used = 0;
    block.freelist = nullptr;

    void* p1 = alloc(&block, 100);
    std::cout << "p1 = " << p1 << '\n';

    dealloc(&block, p1);
    std::cout << "Freed p1\n";

    void* p2 = alloc(&block, 40);
    std::cout << "p2 = " << p2 << '\n';

    if(p1 == p2) {
        std::cout << "Reused block\n";
    }

    void* p3 = alloc(&block, 80);
    std::cout << "p3 = " << p3 << '\n';

    if(p3 == p1) {
        std::cout << "Reused freed block\n";
    } else {
        std::cout << "Did not reuse freed block\n";
    }
    return 0;
}