#pragma once

#include <cstddef>
#include <cstdint>

struct Header {
    size_t size; //total blocks size (sizeof(header) + bytes) inside of the main chunk
    size_t padding; //to make it 8 bytes
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

size_t align_bytes(size_t bytes);
void* alloc(Block* block, size_t bytes);
void dealloc(Block* block, void* ptr);
void* bump_alloc(Block* block, size_t bytes);