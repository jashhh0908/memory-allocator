#include "allocator.h"
#include <iostream>

void alloc_test(Block block);
void sort_test(Block block);
void coalesce_test(Block block);
void calloc_test(Block block);

int main() {
    Block block;
    char memory[1024];

    block.ptr = memory;
    block.capacity = 1024;
    block.used = 0;
    block.freelist = nullptr;
    
    calloc_test(block);
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
