#include<iostream>
#include <cstddef>
#include <cstdint>

struct Header {
    size_t size; // total blocks size (sizeof(header) + bytes) inside of the main chunk
};

struct FreeBlock {
    Header header;
    FreeBlock* next; //store address of next free block
};

struct Block {
    void* ptr; //starting address of the block
    size_t capacity; //total size 
    size_t used; //amt of the size used
    FreeBlock *freelist; //track the amount of free blocks
};

void* allocate(Block *block, size_t bytes) {
    //1. Search the free list
    FreeBlock* prev = nullptr;
    FreeBlock* temp = block->freelist;
    size_t required = sizeof(Header) + bytes;
    while(temp) {
        if(temp->header.size >= required) {
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    if(temp) {
        if(temp == block->freelist) {
            //first block is free
            block->freelist = temp->next;
        } else {
            prev->next = temp->next;
        }
        uintptr_t addr = (uintptr_t)temp + sizeof(Header);
        return (void*)addr;
    } else {
        //2. If no free blocks are there, allocate normally
        //take the size of the maximum alignment for allocation
        const size_t ALIGNMENT = alignof(std::max_align_t); 
        
        //uintptr_t lets us perform pointer arithmetic
        uintptr_t current_addr = (uintptr_t)block->ptr + block->used;
        size_t total_size = sizeof(Header) + bytes;

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

        if(total_size + padding > block->capacity - block->used) {
            std::cout << "Allocation failed: insufficient memory" << std::endl;
            return nullptr;
        }
        Header *h = (Header*)addr;
        h->size = total_size;
        block->used += total_size + padding;
        return (void*)(h + 1); // moves the pointer by sizeof(Header)
    }
}

void free(Block *block, void *ptr) {
    Header* h = ((Header*)ptr - 1);
    FreeBlock* add = (FreeBlock*)h;
    add->next = block->freelist;
    block->freelist = add; 
}

int main() {
    Block block;
    char memory[1024];

    block.ptr = memory;
    block.capacity = 1024;
    block.used = 0;
    block.freelist = nullptr;

    void* p1 = allocate(&block, 100);
    void* p2 = allocate(&block, 50);
    
    std::cout << "p1 = " << p1 << '\n';
    std::cout << "p2 = " << p2 << '\n';
    
    free(&block, p1);
    std::cout << "Freed p1\n";

    void* p3 = allocate(&block, 80);
    std::cout << "p3 = " << p3 << '\n';

    if(p3 == p1) {
        std::cout << "Reused freed block\n";
    } else {
        std::cout << "Did not reuse freed block\n";
    }
    return 0;
}