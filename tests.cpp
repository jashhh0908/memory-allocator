#include "allocator.h"
#include <iostream>

void alloc_test(Block block);
void sort_test(Block block);
void coalesce_test(Block block);
void calloc_test(Block block);
void realloc_test(Block block);
void stats_test(Block block);

int main() {
    Block block = js_getBlock(1024);

    realloc_test(block);
    js_freeBlock(&block);
    return 0;
}

void calloc_test(Block block) {
    // Test 1: int array
    int* arr = (int*)js_calloc(&block, 10, sizeof(int));
    std::cout << "Test 1 (int array): ";
    bool pass = true;
    for(int i = 0; i < 10; i++) {
        if(arr[i] != 0) {
            pass = false;
            break;
        }
    }
    std::cout << (pass ? "PASS" : "FAIL") << '\n';

    // Test 2: char array
    char* chars = (char*)js_calloc(&block, 5, sizeof(char));
    std::cout << "Test 2 (char array): ";
    pass = true;
    for(int i = 0; i < 5; i++) {
        if(chars[i] != 0) {
            pass = false;
            break;
        }
    }
    std::cout << (pass ? "PASS" : "FAIL") << '\n';

    // Test 3: struct
    struct Test {
        int a;
        int b;
    };

    Test* t = (Test*)js_calloc(&block, 1, sizeof(Test));
    std::cout << "Test 3 (struct): ";
    if(t->a == 0 && t->b == 0) {
        std::cout << "PASS\n";
    } else {
        std::cout << "FAIL\n";
    }

    // Test 4: free + reuse + calloc
    int* p1 = (int*)js_calloc(&block, 10, sizeof(int));
    for(int i = 0; i < 10; i++) {
        p1[i] = 123;
    }
    js_dealloc(&block, p1);
    int* p2 = (int*)js_calloc(&block, 10, sizeof(int));
    pass = true;
    for(int i = 0; i < 10; i++) {
        if(p2[i] != 0) {
            pass = false;
            break;
        }
    }

    std::cout << "Test 4 (reuse + zeroing): " << (pass ? "PASS" : "FAIL") << '\n';

    // Test 5: overflow
    void* overflow = js_calloc(&block, SIZE_MAX, 2);
    std::cout << "Test 5 (overflow protection): " << (overflow == nullptr ? "PASS" : "FAIL") << '\n';

    // Test 6: zero-size requests
    void* z1 = js_calloc(&block, 0, 10);
    void* z2 = js_calloc(&block, 10, 0);
    void* z3 = js_calloc(&block, 0, 0);
    std::cout << "Test 6 (zero-size requests): " << z1 << " " << z2 << " " << z3 << '\n';

}

void alloc_test(Block block) {
    void* p1 = js_alloc(&block, 100);
    std::cout << "p1 = " << p1 << '\n';

    js_dealloc(&block, p1);
    std::cout << "Freed p1\n";

    void* p2 = js_alloc(&block, 40);
    std::cout << "p2 = " << p2 << '\n';

    if(p1 == p2) {
        std::cout << "Reused block\n";
    }

    void* p3 = js_alloc(&block, 80);
    std::cout << "p3 = " << p3 << '\n';

    if(p3 == p1) {
        std::cout << "Reused freed block\n";
    } else {
        std::cout << "Did not reuse freed block\n";
    }
}

void sort_test(Block block) {
    void* p1 = js_alloc(&block, 100);
    void* p2 = js_alloc(&block, 100);
    void* p3 = js_alloc(&block, 100);
    void* p4 = js_alloc(&block, 100);

    js_dealloc(&block, p2);
    js_dealloc(&block, p1);
    js_dealloc(&block, p3);

    std::cout << "\nFree list:\n";

    FreeBlock* curr = block.freelist;
    while(curr) {
        std::cout << (void*)curr << '\n';
        curr = curr->next;
    }
}

void coalesce_test(Block block) {
    void* p1 = js_alloc(&block, 100);
    void* p2 = js_alloc(&block, 100);
    void* p3 = js_alloc(&block, 100);

    js_dealloc(&block, p2);

    std::cout << "After freeing p2:\n";
    for(FreeBlock* curr = block.freelist; curr; curr = curr->next) {
        std::cout << curr << " size = " << curr->header.size << '\n';
    }

    std::cout << std::endl;
    js_dealloc(&block, p3);
    
    std::cout << "\nAfter freeing p3:\n";
    for(FreeBlock* curr = block.freelist; curr; curr = curr->next) {
        std::cout << curr << " size = " << curr->header.size << '\n';
    }
}

void realloc_test(Block) {

// Test 1: realloc(nullptr, size)
{
    Block block = js_getBlock(1024);
    int* p = (int*)js_realloc(&block, nullptr, 40);

    std::cout << "Test 1 (nullptr realloc): "
              << (p != nullptr ? "PASS" : "FAIL") << '\n';

    js_freeBlock(&block);
}

// Test 2: grow in place
{
    Block block = js_getBlock(1024);
    bool pass = true;

    int* grow = (int*)js_alloc(&block, 40);
    for(int i = 0; i < 10; i++) grow[i] = i;

    void* adjacent = js_alloc(&block, 100);
    js_dealloc(&block, adjacent);

    int* old = grow;
    grow = (int*)js_realloc(&block, grow, 80);

    pass = (grow == old);

    for(int i = 0; i < 10; i++) {
        if(grow[i] != i) pass = false;
    }

    std::cout << "Test 2 (grow in place): "
              << (pass ? "PASS" : "FAIL") << '\n';
    
    js_freeBlock(&block);
}

// Test 3: grow by moving
{
    Block block = js_getBlock(1024);
    bool pass = true;

    int* move = (int*)js_alloc(&block, 40);

    for(int i = 0; i < 10; i++) {
        move[i] = i + 100;
    }

    js_alloc(&block, 40);

    int* moved = (int*)js_realloc(&block, move, 200);

    pass = (moved != nullptr);

    for(int i = 0; i < 10; i++) {
        if(moved[i] != i + 100) pass = false;
    }

    std::cout << "Test 3 (grow by moving): "
              << (pass ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 4: shrink
{
    Block block = js_getBlock(1024);
    int* shrink = (int*)js_alloc(&block, 128);
    void* before = shrink;

    shrink = (int*)js_realloc(&block, shrink, 32);

    std::cout << "Test 4 (shrink): "
              << ((void*)shrink == before ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 5: same size
{
    Block block = js_getBlock(1024);
    int* same = (int*)js_alloc(&block, 64);
    void* before = same;

    same = (int*)js_realloc(&block, same, 64);

    std::cout << "Test 5 (same size): "
              << ((void*)same == before ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 6: size 0
{
    Block block = js_getBlock(1024);
    int* zero = (int*)js_alloc(&block, 64);

    void* result = js_realloc(&block, zero, 0);

    std::cout << "Test 6 (size 0): "
              << (result == nullptr ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 7: allocation failure
{
    Block block = js_getBlock(1024);
    void* huge = js_alloc(&block, 128);

    void* failed = js_realloc(&block, huge, 100000);

    std::cout << "Test 7 (failure handling): "
              << (failed == nullptr ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 8: data preservation
{
    Block block = js_getBlock(1024);
    bool pass = true;

    char* data = (char*)js_alloc(&block, 16);

    for(int i = 0; i < 16; i++) {
        data[i] = 'A' + i;
    }

    js_alloc(&block, 32);

    char* data2 = (char*)js_realloc(&block, data, 128);

    for(int i = 0; i < 16; i++) {
        if(data2[i] != 'A' + i) pass = false;
    }

    std::cout << "Test 8 (data preservation): "
              << (pass ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 9: grow in place with split
{
    Block block = js_getBlock(1024);
    int* p = (int*)js_alloc(&block, 32);

    void* big = js_alloc(&block, 128);
    js_dealloc(&block, big);

    void* old = p;

    p = (int*)js_realloc(&block, p, 64);

    std::cout << "Test 9 (grow in place with split): "
              << ((void*)p == old ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 10: grow in place without split
{
    Block block = js_getBlock(1024);
    int* p = (int*)js_alloc(&block, 32);

    void* small = js_alloc(&block, 32);
    js_dealloc(&block, small);

    void* old = p;

    p = (int*)js_realloc(&block, p, 56);

    std::cout << "Test 10 (grow in place without split): "
              << ((void*)p == old ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}

// Test 11: repeated realloc chain
{
    Block block = js_getBlock(1024);
    bool pass = true;

    int* p = (int*)js_alloc(&block, 16);

    for(int i = 0; i < 4; i++) {
        p[i] = i + 1;
    }

    p = (int*)js_realloc(&block, p, 64);
    p = (int*)js_realloc(&block, p, 128);
    p = (int*)js_realloc(&block, p, 24);
    p = (int*)js_realloc(&block, p, 80);

    for(int i = 0; i < 4; i++) {
        if(p[i] != i + 1) pass = false;
    }

    std::cout << "Test 11 (repeated realloc chain): "
              << (pass ? "PASS" : "FAIL") << '\n';
    js_freeBlock(&block);
}
}

void stats_test(Block block) {
    js_alloc(&block, 32);
    js_alloc(&block, 64);
    js_alloc(&block, 128);
    js_alloc(&block, block.capacity * 10); // guaranteed fail

    void* p1 = js_alloc(&block, 32);
    void* p2 = js_alloc(&block, 64);
    js_dealloc(&block, p1);
    js_dealloc(&block, p2);

    bool pass1 = (block.stats.total_allocations == 5) && (block.stats.failed_allocations == 1) && (block.stats.total_frees == 2);

    std::cout << "Stats Test 1 (alloc counters): " << (pass1 ? "PASS" : "FAIL") << std::endl;

    std::cout << "Allocations: " << block.stats.total_allocations 
              << ", Failures: " << block.stats.failed_allocations
              << ", Frees: " << block.stats.total_frees << std::endl;

    void* p3 = js_alloc(&block, 15);

    bool pass2 =
        (block.stats.current_allocated_bytes == 239) &&
        (block.stats.peak_allocated_bytes == 320);

    std::cout << "Test 2 (reuse accounting): "
              << (pass2 ? "PASS" : "FAIL") << '\n';

    js_dealloc(&block, p3);

    bool pass3 =
        (block.stats.current_allocated_bytes == 224) &&
        (block.stats.peak_allocated_bytes == 320);

    std::cout << "Test 3 (reuse free): "
              << (pass3 ? "PASS" : "FAIL") << '\n';

    std::cout << "\nCurrent Allocated: "
              << block.stats.current_allocated_bytes << '\n';

    std::cout << "Current Consumed: "
              << block.stats.current_consumed_bytes << '\n';

    std::cout << "Peak Allocated: "
              << block.stats.peak_allocated_bytes << '\n';

    // Realloc metric tests

    void* p4 = js_alloc(&block, 15);

    size_t alloc_before = block.stats.current_allocated_bytes;
    size_t consumed_before = block.stats.current_consumed_bytes;

    p4 = js_realloc(&block, p4, 10);

    bool pass4 =
        (block.stats.current_allocated_bytes == alloc_before - 5) &&
        (block.stats.current_consumed_bytes == consumed_before);

    std::cout << "Test 4 (logical shrink): "
              << (pass4 ? "PASS" : "FAIL") << '\n';

    size_t alloc_before2 = block.stats.current_allocated_bytes;
    size_t consumed_before2 = block.stats.current_consumed_bytes;

    p4 = js_realloc(&block, p4, 16);

    bool pass5 =
        (block.stats.current_allocated_bytes == alloc_before2 + 6) &&
        (block.stats.current_consumed_bytes == consumed_before2);

    std::cout << "Test 5 (logical growth): "
              << (pass5 ? "PASS" : "FAIL") << '\n';

    void* p5 = js_alloc(&block, 128);

    size_t alloc_before3 = block.stats.current_allocated_bytes;
    size_t consumed_before3 = block.stats.current_consumed_bytes;

    p5 = js_realloc(&block, p5, 32);

    bool pass6 =
        (block.stats.current_allocated_bytes == alloc_before3 - 96) &&
        (block.stats.current_consumed_bytes < consumed_before3);

    std::cout << "Test 6 (physical shrink): "
              << (pass6 ? "PASS" : "FAIL") << '\n';

    std::cout << "\nCurrent Allocated: "
              << block.stats.current_allocated_bytes << '\n';

    std::cout << "Current Consumed: "
              << block.stats.current_consumed_bytes << '\n';

    std::cout << "Peak Allocated: "
              << block.stats.peak_allocated_bytes << '\n';

    void* p6 = js_alloc(&block, 32);
    void* adjacent = js_alloc(&block, 128);
    
    js_dealloc(&block, adjacent);
    
    size_t alloc_before4 = block.stats.current_allocated_bytes;
    size_t consumed_before4 = block.stats.current_consumed_bytes;
    size_t peak_before4 = block.stats.peak_allocated_bytes;
    
    p6 = js_realloc(&block, p6, 80);
    
    bool pass7 =
        (block.stats.current_allocated_bytes == alloc_before4 + 48) &&
        (block.stats.current_consumed_bytes > consumed_before4) &&
        (block.stats.peak_allocated_bytes >= peak_before4);
    
    std::cout << "Test 7 (in-place growth): "
              << (pass7 ? "PASS" : "FAIL") << '\n';

    void* p7 = js_alloc(&block, 32);
    js_alloc(&block, 32); // prevent in-place growth

    size_t alloc_before5 = block.stats.current_allocated_bytes;
    size_t consumed_before5 = block.stats.current_consumed_bytes;
    size_t peak_before5 = block.stats.peak_allocated_bytes;

    p7 = js_realloc(&block, p7, 200);

    bool pass8 =
        (block.stats.current_allocated_bytes == alloc_before5 + 168) &&
        (block.stats.current_consumed_bytes > consumed_before5) &&
        (block.stats.peak_allocated_bytes >= peak_before5);

    std::cout << "Test 8 (allocate-copy-free growth): "
              << (pass8 ? "PASS" : "FAIL") << '\n';

    void* p8 = js_alloc(&block, 32);

    size_t alloc_before6 = block.stats.current_allocated_bytes;
    size_t consumed_before6 = block.stats.current_consumed_bytes;
    size_t peak_before6 = block.stats.peak_allocated_bytes;
    size_t failed_before6 = block.stats.failed_allocations;
    
    void* failed = js_realloc(&block, p8, block.capacity * 100);
    
    bool pass9 =
        (failed == nullptr) &&
        (block.stats.current_allocated_bytes == alloc_before6) &&
        (block.stats.current_consumed_bytes == consumed_before6) &&
        (block.stats.peak_allocated_bytes == peak_before6) &&
        (block.stats.failed_allocations == failed_before6 + 1);
    
    std::cout << "Test 9 (failed realloc): "
              << (pass9 ? "PASS" : "FAIL") << '\n';
}

