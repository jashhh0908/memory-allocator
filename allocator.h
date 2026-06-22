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

struct Stats {
    size_t current_allocated_bytes = 0; //bytes allocated to the user (does not include overhead)
    size_t current_consumed_bytes = 0; //bytes allocated to the user + overhead
    size_t peak_allocated_bytes = 0; //max bytes ever requested by the user
    size_t total_allocations = 0;
    size_t total_frees = 0;
    size_t failed_allocations = 0;
};

struct Block {
    void* ptr; //starting address of the block
    size_t capacity; //total size 
    size_t used; //amt of the size used
    FreeBlock *freelist; //track the amount of free blocks
    Stats stats;
};

size_t align_bytes(size_t bytes);
void* js_alloc(Block* block, size_t bytes);
void js_dealloc(Block* block, void* ptr);
void* bump_alloc(Block* block, size_t bytes);
void coalesce(FreeBlock* prev, FreeBlock* add, FreeBlock* curr);
void* js_calloc(Block* block, size_t count, size_t size);
void js_memset(void* ptr, int value, size_t count);
void* js_realloc(Block* block, void* ptr, size_t size);
void js_memcpy( void* dest, void* src, size_t count);