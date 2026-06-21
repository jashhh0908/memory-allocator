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

void* js_alloc(Block *block, size_t bytes) {
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

void coalesce(FreeBlock* prev, FreeBlock* add, FreeBlock* curr) {
    if(curr == nullptr) {
        uintptr_t end_prev = (uintptr_t)prev + prev->header.size;
        if(end_prev == (uintptr_t)add) {
            prev->header.size += add->header.size;
            prev->next = add->next;
        }
        return;
    }
    uintptr_t end_add = (uintptr_t)add + add->header.size;
    if(end_add == (uintptr_t)curr) {
        add->header.size += curr->header.size;
        add->next = curr->next;
    }
}

void insert_free_block(Block* block, FreeBlock* add) {
    add->next = nullptr;
    if(!block->freelist) {
        block->freelist = add;
        return;
    } 
    if((uintptr_t)add < (uintptr_t)block->freelist) {
        FreeBlock* old = block->freelist;
        add->next = old;
        block->freelist = add;
        coalesce(nullptr, add, old);
        return;
    }
    FreeBlock* prev = nullptr;
    FreeBlock* curr = block->freelist;
    while(curr) {
        if((uintptr_t)curr > (uintptr_t)add) {
            break;
        }
        prev = curr;
        curr = curr->next;
    } 
    if(curr == nullptr) {
        prev->next = add;
        coalesce(prev, add, nullptr);
        return;
    }
    prev->next = add;
    add->next = curr;
    coalesce(prev, add, curr);
}

void js_dealloc(Block *block, void *ptr) {
    Header* h = ((Header*)ptr - 1);
    FreeBlock* add = (FreeBlock*)h;
    insert_free_block(block, add);
}   

void js_memset(void* ptr, int value, size_t count) {
    unsigned char* p = (unsigned char*)ptr;
    for(size_t i = 0; i < count; i++) {
        p[i] = value;
    }
}

void* js_calloc(Block* block, size_t count, size_t size) {
    //check if the total size requested is within the limit
    if(size != 0 && count > SIZE_MAX / size) {
        return nullptr;
    }
    size_t total = count * size;
    void* ptr = js_alloc(block, total);
    if(!ptr) {
        std::cout << "calloc failed\n";
        return nullptr;
    }
    js_memset(ptr, 0, total);
    return ptr;
}

void js_memcpy( void* dest, void* src, size_t count) {
    unsigned char* s = (unsigned char*)src;
    unsigned char* d = (unsigned char*)dest;
    for(size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
}

void* allocate_copy_free(Block* block, void* ptr, size_t alloc_size, size_t copy_size) {
    //allocate new block
    void* new_ptr = js_alloc(block, alloc_size);
    //edge case
    if(!new_ptr) {
        std::cout << "Realloc copy free failed\n";
        return nullptr;
    }
    //copy old contents
    js_memcpy(new_ptr, ptr, copy_size);
    //free old block    
    js_dealloc(block, ptr);
    //return
    return new_ptr;
}

void* js_realloc(Block* block, void* ptr, size_t size) {
    //if first time allocation, pass to malloc
    if(!ptr) {
        return js_alloc(block, size);
    }
    //check size
    if(size == 0) {
        js_dealloc(block, ptr);
        return js_alloc(block, size);
    }
    Header* h = ((Header*)ptr - 1);
    size_t current_payload = h->size - sizeof(Header);
    //compare current ptr size and realloc size to check if memory needs to be shrunken or grown
    if(current_payload >= size) { // check for shrinking
        if(current_payload == size) {
            return ptr;
        } 
        size_t required = sizeof(Header) + align_bytes(size);
        size_t remaining = h->size - required;
        if(remaining >= sizeof(FreeBlock)) {
            //shrink current block to that size
            h->size = required;
            //if remaining is greater than we can split it and add remaining to free list.
            //compute the address of the new free block 
            uintptr_t leftover_addr = (uintptr_t)h + required;
            FreeBlock* leftover = (FreeBlock*)leftover_addr;
            leftover->header.size = remaining;
            //insert leftover in free list
            insert_free_block(block, leftover);
        }
        return ptr;
    }
    //growing the memory
    //get the next block addr and check if its free
    uintptr_t next_addr = (uintptr_t)h + h->size;
    FreeBlock* prev = nullptr;
    FreeBlock* temp = block->freelist;
    while(temp) {
        if(next_addr == (uintptr_t)temp) {
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    if(!temp) {
        //next block is not free, need to move ptr to the location where requested size can be allocated
        size_t min_size = (current_payload < size) ? current_payload : size;
        return allocate_copy_free(block, ptr, size, min_size);
    } else {
        //next block is free
        size_t combined = h->size + temp->header.size;
        size_t required = sizeof(Header) + align_bytes(size);
        if(combined >= required) {
            //grow the current block to fit the size 
            size_t remaining = combined - required;
            if(remaining >= sizeof(FreeBlock)) {
                //the next block has extra memory, where some part can be allocated and the rest can be free
                h->size = required; 
                //delete temp from free list 
                if(temp == block->freelist) {
                    block->freelist = temp->next;
                } else {
                    prev->next = temp->next;
                }
                uintptr_t leftover_addr = (uintptr_t)h + required;
                FreeBlock* leftover = (FreeBlock*)leftover_addr;
                leftover->header.size = remaining;  
                insert_free_block(block, leftover);
            } else {
                //resize h to fill full combined bytes
                //delete temp
                if(temp == block->freelist) {
                    block->freelist = temp->next;
                } else {
                    prev->next = temp->next;
                }
                h->size = combined;
            }
            return ptr;
        } else {
            size_t min_size = (current_payload < size) ? current_payload : size;
            return allocate_copy_free(block, ptr, size, min_size);
        }
    }
}
