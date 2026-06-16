#include<iostream>
#include <cstddef>
#include <cstdint>

struct Block {
    void* ptr; //starting address of the block
    size_t capacity; //total size 
    size_t used; //amt of the size used
};

void* allocate(Block *block, size_t bytes) {
    //take the size of the maximum alignment for allocation
    const size_t ALIGNMENT = alignof(std::max_align_t); 
    
    //uintptr_t lets us perform pointer arithmetic
    uintptr_t current_addr = (uintptr_t)block->ptr + block->used;

    /*  
        We align the addresses to make it optimal for the CPU to access the bytes.
        Allocations are rounded up so that the data types can start at addresses
        boundaries divisible by the maximum alignment (max_align_t)   
        
        There are 2 methods of aligning the address in order to allocate optimally 
        1.  Compute the extra padding bits which need to be ignored 
            size_t rem = ptr % ALIGNMENT;
            size_t padding = (ALIGNMENT - rem) % ALIGNMENT;
            uintptr_t addr = ptr + padding; 

        2.  Using bit operations to get the next adddress that is divisible with max_align_t
    */

    uintptr_t addr = (current_addr + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    size_t padding = addr - current_addr;

    if(bytes + padding > block->capacity - block->used) {
        std::cout << "Allocation failed: insufficient memory" << std::endl;
        return nullptr;
    }
    block->used += bytes + padding;
    return (void*)addr;
}

int main() {
    Block block;
    char memory[1024];
    
    block.ptr = memory;
    block.capacity = 1024;
    block.used = 0;

    void* p1 = allocate(&block, 100);
    void* p2 = allocate(&block, 50);
    void* p3 = allocate(&block, 200);
    
    std::cout << p1 << '\n';
    std::cout << p2 << '\n';
    std::cout << p3 << '\n';
    
    std::cout << block.used << '\n';

    char* c = (char*)allocate(&block, 1);
    int* i = (int*)allocate(&block, sizeof(int));
    double* d = (double*)allocate(&block, sizeof(double));
    
    std::cout << ((uintptr_t)i % alignof(int)) << '\n';
    std::cout << ((uintptr_t)d % alignof(double)) << '\n';
    
    return 0;
}