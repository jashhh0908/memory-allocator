#include "allocator.h"
#include <iostream>

void alloc_test();
void sort_test();
void coalesce_test();
void calloc_test();
void realloc_test();
void stats_test();

int main() {
    alloc_test();
    js_reset_allocator();

    sort_test();
    js_reset_allocator();

    coalesce_test();
    js_reset_allocator();

    calloc_test();
    js_reset_allocator();

    realloc_test();
    js_reset_allocator();

    stats_test();
    js_reset_allocator();
    return 0;
}

void calloc_test() {
    // Test 1: int array
    int* arr = (int*)js_calloc(10, sizeof(int));
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
    char* chars = (char*)js_calloc(5, sizeof(char));
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

    Test* t = (Test*)js_calloc(1, sizeof(Test));
    std::cout << "Test 3 (struct): ";
    if(t->a == 0 && t->b == 0) {
        std::cout << "PASS\n";
    } else {
        std::cout << "FAIL\n";
    }

    // Test 4: free + reuse + calloc
    int* p1 = (int*)js_calloc(10, sizeof(int));
    for(int i = 0; i < 10; i++) {
        p1[i] = 123;
    }
    js_dealloc(p1);
    int* p2 = (int*)js_calloc(10, sizeof(int));
    pass = true;
    for(int i = 0; i < 10; i++) {
        if(p2[i] != 0) {
            pass = false;
            break;
        }
    }

    std::cout << "Test 4 (reuse + zeroing): " << (pass ? "PASS" : "FAIL") << '\n';

    // Test 5: overflow
    void* overflow = js_calloc(SIZE_MAX, 2);
    std::cout << "Test 5 (overflow protection): " << (overflow == nullptr ? "PASS" : "FAIL") << '\n';

    // Test 6: zero-size requests
    void* z1 = js_calloc(0, 10);
    void* z2 = js_calloc(10, 0);
    void* z3 = js_calloc(0, 0);
    std::cout << "Test 6 (zero-size requests): " << z1 << " " << z2 << " " << z3 << '\n';

}

void alloc_test() {
    void* p1 = js_alloc(100);
    std::cout << "p1 = " << p1 << '\n';

    js_dealloc(p1);
    std::cout << "Freed p1\n";

    void* p2 = js_alloc(40);
    std::cout << "p2 = " << p2 << '\n';

    if(p1 == p2) {
        std::cout << "Reused block\n";
    }

    void* p3 = js_alloc(80);
    std::cout << "p3 = " << p3 << '\n';

    if(p3 == p1) {
        std::cout << "Reused freed block\n";
    } else {
        std::cout << "Did not reuse freed block\n";
    }
}

void sort_test() {
    void* p1 = js_alloc(100);
    void* p2 = js_alloc(100);
    void* p3 = js_alloc(100);
    void* p4 = js_alloc(100);

    js_dealloc(p2);
    js_dealloc(p1);
    js_dealloc(p3);

    std::cout << "\nFree list:\n";

    FreeBlock* curr = js_get_freelist();
    while(curr) {
        std::cout << (void*)curr << '\n';
        curr = curr->next;
    }
}

void coalesce_test() {
    void* p1 = js_alloc(100);
    void* p2 = js_alloc(100);
    void* p3 = js_alloc(100);

    js_dealloc(p2);

    std::cout << "After freeing p2:\n";
    for(FreeBlock* curr = js_get_freelist(); curr; curr = curr->next) {
        std::cout << curr << " size = " << curr->header.size << '\n';
    }

    std::cout << std::endl;
    js_dealloc(p3);
    
    std::cout << "\nAfter freeing p3:\n";
    for(FreeBlock* curr = js_get_freelist(); curr; curr = curr->next) {
        std::cout << curr << " size = " << curr->header.size << '\n';
    }
}

void realloc_test() {

// Test 1: realloc(nullptr, size)
{
    int* p = (int*)js_realloc(nullptr, 40);

    std::cout << "Test 1 (nullptr realloc): " << (p != nullptr ? "PASS" : "FAIL") << '\n';
}

// Test 2: grow in place
{
    bool pass = true;

    int* grow = (int*)js_alloc(40);
    for(int i = 0; i < 10; i++) grow[i] = i;

    void* adjacent = js_alloc(100);
    js_dealloc(adjacent);

    int* old = grow;
    grow = (int*)js_realloc(grow, 80);

    pass = (grow == old);

    for(int i = 0; i < 10; i++) {
        if(grow[i] != i) pass = false;
    }

    std::cout << "Test 2 (grow in place): " << (pass ? "PASS" : "FAIL") << '\n';
}

// Test 3: grow by moving
{
    bool pass = true;

    int* move = (int*)js_alloc(40);

    for(int i = 0; i < 10; i++) {
        move[i] = i + 100;
    }

    js_alloc(40);

    int* moved = (int*)js_realloc(move, 200);

    pass = (moved != nullptr);

    for(int i = 0; i < 10; i++) {
        if(moved[i] != i + 100) pass = false;
    }

    std::cout << "Test 3 (grow by moving): " << (pass ? "PASS" : "FAIL") << '\n';
}

// Test 4: shrink
{
    int* shrink = (int*)js_alloc(128);
    void* before = shrink;

    shrink = (int*)js_realloc(shrink, 32);

    std::cout << "Test 4 (shrink): " << ((void*)shrink == before ? "PASS" : "FAIL") << '\n';
}

// Test 5: same size
{
    int* same = (int*)js_alloc(64);
    void* before = same;

    same = (int*)js_realloc(same, 64);

    std::cout << "Test 5 (same size): " << ((void*)same == before ? "PASS" : "FAIL") << '\n';
}

// Test 6: size 0
{
    int* zero = (int*)js_alloc(64);

    void* result = js_realloc(zero, 0);

    std::cout << "Test 6 (size 0): " << (result == nullptr ? "PASS" : "FAIL") << '\n';
}

// Test 7: allocation failure
{
    void* huge = js_alloc(128);

    void* failed = js_realloc(huge, 100000);

    std::cout << "Test 7 (failure handling): " << (failed == nullptr ? "PASS" : "FAIL") << '\n';
}

// Test 8: data preservation
{
    bool pass = true;

    char* data = (char*)js_alloc(16);

    for(int i = 0; i < 16; i++) {
        data[i] = 'A' + i;
    }

    js_alloc(32);

    char* data2 = (char*)js_realloc(data, 128);

    for(int i = 0; i < 16; i++) {
        if(data2[i] != 'A' + i) pass = false;
    }

    std::cout << "Test 8 (data preservation): " << (pass ? "PASS" : "FAIL") << '\n';
}

// Test 9: grow in place with split
{
    int* p = (int*)js_alloc(32);

    void* big = js_alloc(128);
    js_dealloc(big);

    void* old = p;

    p = (int*)js_realloc(p, 64);

    std::cout << "Test 9 (grow in place with split): " << ((void*)p == old ? "PASS" : "FAIL") << '\n';
}

// Test 10: grow in place without split
{
    int* p = (int*)js_alloc(32);

    void* small = js_alloc(32);
    js_dealloc(small);

    void* old = p;

    p = (int*)js_realloc(p, 56);

    std::cout << "Test 10 (grow in place without split): " << ((void*)p == old ? "PASS" : "FAIL") << '\n';
}

// Test 11: repeated realloc chain
{
    bool pass = true;

    int* p = (int*)js_alloc(16);

    for(int i = 0; i < 4; i++) {
        p[i] = i + 1;
    }

    p = (int*)js_realloc(p, 64);
    p = (int*)js_realloc(p, 128);
    p = (int*)js_realloc(p, 24);
    p = (int*)js_realloc(p, 80);

    for(int i = 0; i < 4; i++) {
        if(p[i] != i + 1) pass = false;
    }

    std::cout << "Test 11 (repeated realloc chain): " << (pass ? "PASS" : "FAIL") << '\n';
}
}

void stats_test() {
    js_alloc(32);
    js_alloc(64);
    js_alloc(128);
    js_alloc(js_get_capacity() * 10); // guaranteed fail

    void* p1 = js_alloc(32);
    void* p2 = js_alloc(64);
    js_dealloc(p1);
    js_dealloc(p2);

    const Stats& stats = get_stats();
    bool pass1 = (stats.total_allocations == 5) && (stats.failed_allocations == 1) && (stats.total_frees == 2);

    std::cout << "Stats Test 1 (alloc counters): " << (pass1 ? "PASS" : "FAIL") << std::endl;

    std::cout << "Allocations: " << stats.total_allocations 
              << ", Failures: " << stats.failed_allocations
              << ", Frees: " << stats.total_frees << std::endl;

    void* p3 = js_alloc(15);

    bool pass2 = (stats.current_allocated_bytes == 239) && (stats.peak_allocated_bytes == 320);
    std::cout << "Test 2 (reuse accounting): " << (pass2 ? "PASS" : "FAIL") << '\n';

    js_dealloc(p3);
    bool pass3 = (stats.current_allocated_bytes == 224) && (stats.peak_allocated_bytes == 320);

    std::cout << "Test 3 (reuse free): " << (pass3 ? "PASS" : "FAIL") << '\n';

    std::cout << "\nCurrent Allocated: " << stats.current_allocated_bytes << '\n';

    std::cout << "Current Consumed: " << stats.current_consumed_bytes << '\n';

    std::cout << "Peak Allocated: " << stats.peak_allocated_bytes << '\n';

    // Realloc metric tests

    void* p4 = js_alloc(15);

    size_t alloc_before = stats.current_allocated_bytes;
    size_t consumed_before = stats.current_consumed_bytes;

    p4 = js_realloc(p4, 10);

    bool pass4 = (stats.current_allocated_bytes == alloc_before - 5) && (stats.current_consumed_bytes == consumed_before);

    std::cout << "Test 4 (logical shrink): " << (pass4 ? "PASS" : "FAIL") << '\n';

    size_t alloc_before2 = stats.current_allocated_bytes;
    size_t consumed_before2 = stats.current_consumed_bytes;

    p4 = js_realloc(p4, 16);

    bool pass5 = (stats.current_allocated_bytes == alloc_before2 + 6) && (stats.current_consumed_bytes == consumed_before2);

    std::cout << "Test 5 (logical growth): " << (pass5 ? "PASS" : "FAIL") << '\n';

    void* p5 = js_alloc(128);

    size_t alloc_before3 = stats.current_allocated_bytes;
    size_t consumed_before3 = stats.current_consumed_bytes;

    p5 = js_realloc(p5, 32);

    bool pass6 = (stats.current_allocated_bytes == alloc_before3 - 96) && (stats.current_consumed_bytes < consumed_before3);

    std::cout << "Test 6 (physical shrink): " << (pass6 ? "PASS" : "FAIL") << '\n';

    std::cout << "\nCurrent Allocated: " << stats.current_allocated_bytes << '\n';

    std::cout << "Current Consumed: " << stats.current_consumed_bytes << '\n';

    std::cout << "Peak Allocated: " << stats.peak_allocated_bytes << '\n';

    void* p6 = js_alloc(32);
    void* adjacent = js_alloc(128);
    
    js_dealloc(adjacent);
    
    size_t alloc_before4 = stats.current_allocated_bytes;
    size_t consumed_before4 = stats.current_consumed_bytes;
    size_t peak_before4 = stats.peak_allocated_bytes;
    
    p6 = js_realloc(p6, 80);
    
    bool pass7 =
        (stats.current_allocated_bytes == alloc_before4 + 48) &&
        (stats.current_consumed_bytes > consumed_before4) &&
        (stats.peak_allocated_bytes >= peak_before4);
    
    std::cout << "Test 7 (in-place growth): " << (pass7 ? "PASS" : "FAIL") << '\n';

    void* p7 = js_alloc(32);
    js_alloc(32); // prevent in-place growth

    size_t alloc_before5 = stats.current_allocated_bytes;
    size_t consumed_before5 = stats.current_consumed_bytes;
    size_t peak_before5 = stats.peak_allocated_bytes;

    p7 = js_realloc(p7, 200);

    bool pass8 =
        (stats.current_allocated_bytes == alloc_before5 + 168) &&
        (stats.current_consumed_bytes > consumed_before5) &&
        (stats.peak_allocated_bytes >= peak_before5);

    std::cout << "Test 8 (allocate-copy-free growth): " << (pass8 ? "PASS" : "FAIL") << '\n';

    void* p8 = js_alloc(32);

    size_t alloc_before6 = stats.current_allocated_bytes;
    size_t consumed_before6 = stats.current_consumed_bytes;
    size_t peak_before6 = stats.peak_allocated_bytes;
    size_t failed_before6 = stats.failed_allocations;
    
    void* failed = js_realloc(p8, js_get_capacity() * 100);
    
    bool pass9 =
        (failed == nullptr) &&
        (stats.current_allocated_bytes == alloc_before6) &&
        (stats.current_consumed_bytes == consumed_before6) &&
        (stats.peak_allocated_bytes == peak_before6) &&
        (stats.failed_allocations == failed_before6 + 1);
    
    std::cout << "Test 9 (failed realloc): " << (pass9 ? "PASS" : "FAIL") << '\n';
}