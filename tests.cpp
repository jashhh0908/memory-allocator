#include "allocator.h"
#include <iostream>

void alloc_test(Block block);
void sort_test(Block block);
void coalesce_test(Block block);

int main() {
    Block block;
    char memory[1024];

    block.ptr = memory;
    block.capacity = 1024;
    block.used = 0;
    block.freelist = nullptr;
    
    coalesce_test(block);
    return 0;
}

void alloc_test(Block block) {
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
}

void sort_test(Block block) {
    void* p1 = alloc(&block, 100);
    void* p2 = alloc(&block, 100);
    void* p3 = alloc(&block, 100);
    void* p4 = alloc(&block, 100);

    dealloc(&block, p2);
    dealloc(&block, p1);
    dealloc(&block, p3);

    std::cout << "\nFree list:\n";

    FreeBlock* curr = block.freelist;
    while(curr) {
        std::cout << (void*)curr << '\n';
        curr = curr->next;
    }
}

void coalesce_test(Block block) {
    void* p1 = alloc(&block, 100);
    void* p2 = alloc(&block, 100);
    void* p3 = alloc(&block, 100);

    dealloc(&block, p2);

    std::cout << "After freeing p2:\n";
    for(FreeBlock* curr = block.freelist; curr; curr = curr->next) {
        std::cout << curr << " size = " << curr->header.size << '\n';
    }

    std::cout << std::endl;
    dealloc(&block, p3);
    
    std::cout << "\nAfter freeing p3:\n";
    for(FreeBlock* curr = block.freelist; curr; curr = curr->next) {
        std::cout << curr << " size = " << curr->header.size << '\n';
    }
}
