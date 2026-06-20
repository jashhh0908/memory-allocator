#include "allocator.h"
#include <iostream>

size_t align_bytes(size_t bytes) {
    const size_t ALIGNMENT = alignof(std::max_align_t); 
    return ((bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1));
}

void* bump_alloc(Block* block, size_t bytes) {
    //uintptr_t lets us perform pointer arithmetic
    uintptr_t current_addr = (uintptr_t)block->ptr + block->used;
    size_t total_size = sizeof(Header) + align_bytes(bytes);
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
    uintptr_t addr = align_bytes(current_addr);
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

void* alloc(Block *block, size_t bytes) {
    //1. Search the free list
    FreeBlock* prev = nullptr;
    FreeBlock* temp = block->freelist;
    //take the size of the maximum alignment for allocation
    size_t required = sizeof(Header) + align_bytes(bytes);
    while(temp) {
        if(temp->header.size >= required) {
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    if(temp) {
        size_t remaining_size = temp->header.size - required;
        temp->header.size = required;
        FreeBlock* next = temp->next;
        if(remaining_size >= sizeof(FreeBlock)) {
            uintptr_t leftover_addr = (uintptr_t)temp + required;
            FreeBlock* leftover = (FreeBlock*)leftover_addr;
            leftover->header.size = remaining_size;
            if(temp == block->freelist) {
                block->freelist = leftover;
            } else {
                prev->next = leftover;
            }
            leftover->next = next;
            return (void*)((uintptr_t)temp + sizeof(Header));
        } else {
            if(temp == block->freelist) {
                //first block is free
                block->freelist = temp->next;
            } else {
                prev->next = temp->next;
            }
            return (void*)((uintptr_t)temp + sizeof(Header));
        }
    } else {
        //2. If no free blocks are there, allocate normally
        return bump_alloc(block, bytes);
    }
}

void dealloc(Block *block, void *ptr) {
    Header* h = ((Header*)ptr - 1);
    FreeBlock* add = (FreeBlock*)h;
    add->next = block->freelist;
    block->freelist = add; 
}