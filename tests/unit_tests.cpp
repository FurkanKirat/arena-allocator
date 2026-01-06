#include <cstdlib>
#include <iostream>
#include "arena_allocator.h"

#define TEST_ASSERT(cond,msg) \
    if (!(cond)) { \
        std::cerr << "[FAIL] " << msg << " in " << __FILE__ << ":" << __LINE__ << "\n"; \
        std::exit(1); \
    } else { \
        std::cout << "[PASS] " << msg << "\n"; \
    }

void TestAlignment() {
    ArenaAllocator arena = ArenaAllocator(1024);

    // Test Case: Verify that allocations respect memory alignment requirements.
    // Misaligned memory can cause performance penalties or crashes on some architectures (e.g., ARM).
    char* c = arena.New<char>('A');

    double* d = arena.New<double>(3.14);

    uintptr_t addr = reinterpret_cast<uintptr_t>(d);
    TEST_ASSERT(addr % alignof(double) == 0, "Address must be aligned to 8 bytes");
}

void TestOutOfMemory() {
    ArenaAllocator arena(100);

    // Test Case: Ensure the allocator gracefully handles OOM (Out Of Memory) scenarios
    // by returning nullptr instead of causing a buffer overflow.
    void* block1 = arena.Alloc(80);
    TEST_ASSERT(block1 != nullptr, "First allocation should succeed");

    void* block2 = arena.Alloc(50);
    TEST_ASSERT(block2 == nullptr, "Allocation exceeding capacity should return nullptr");
}

void TestReset() {
    ArenaAllocator arena(1024);

    void* firstAddress = arena.Alloc(100);
    arena.Reset();
    void* secondAddress = arena.Alloc(100);

    TEST_ASSERT(firstAddress == secondAddress, "After Reset, allocator should start from the beginning");
}

void TestArrayAllocation() {
    ArenaAllocator arena(1024);

    int count = 5;
    int* numbers = arena.AllocArray<int>(count);

    TEST_ASSERT(numbers != nullptr, "Array allocation should succeed");

    for (int i = 0; i < count; i++) {
        numbers[i] = i * 10;
    }

    TEST_ASSERT(numbers[4] == 40, "Should be able to read/write last element");

    uintptr_t addr0 = reinterpret_cast<uintptr_t>(&numbers[0]);
    uintptr_t addr1 = reinterpret_cast<uintptr_t>(&numbers[1]);

    TEST_ASSERT(addr1 - addr0 == sizeof(int), "Array elements must be contiguous");
}

struct TestObj {
    bool* isDestructed;
    explicit TestObj(bool* flag) : isDestructed(flag) { *isDestructed = false; }
    ~TestObj() { *isDestructed = true; }
};

void TestNoDestructorCallOnReset() {
    ArenaAllocator arena(1024);
    // Test Case: Verify awareness that Arena::Reset() does NOT call destructors.
    // In arena-based systems, objects are typically PODs or destructed manually.
    bool isDestructed = false;

    TestObj* obj = arena.New<TestObj>(&isDestructed);
    arena.Reset();

    TEST_ASSERT(isDestructed == false, "Arena reset should NOT call destructors automatically");
}

int main() {
    std::cout << "Running Unit Tests...\n";

    TestAlignment();
    TestOutOfMemory();
    TestReset();
    TestArrayAllocation();
    TestNoDestructorCallOnReset();

    std::cout << "All Tests Passed!\n";
    return 0;
}