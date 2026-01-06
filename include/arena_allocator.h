#pragma once
#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <cstddef>
#include <utility>
#include <new>

/**
 * @brief Fast linear allocator for temporary allocations
 * @warning Not thread-safe. Destructors are not called on Reset().
 */
class ArenaAllocator {
public:

    // Constructor
    explicit ArenaAllocator(const size_t sizeInBytes) {
        // Allocating raw memory block. static_cast is necessary because malloc returns void*.
        m_memoryBlock = static_cast<std::byte *>(malloc(sizeInBytes));
        m_totalSize = sizeInBytes;

        // Ideally, we could verify allocation success here (check for nullptr).
        if (!m_memoryBlock) throw std::bad_alloc();
    }

    // Destructor
    ~ArenaAllocator() {
        // RAII Principle: The arena owns the memory, so it must release it upon destruction.
        free(m_memoryBlock);
    }

    // Deleted copy constructor
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    // Move Constructor
    ArenaAllocator(ArenaAllocator &&other) noexcept {

        // Transfer ownership of the memory block without copying data (O(1) operation).
        this->m_memoryBlock = other.m_memoryBlock;
        this->m_totalSize = other.m_totalSize;
        this->m_offset = other.m_offset;

        other.m_memoryBlock = nullptr;
        other.m_totalSize = 0;
        other.m_offset = 0;
    }

    // Assign operator
    ArenaAllocator &operator=(ArenaAllocator &&other) noexcept {
        if (this != &other) {
            free(m_memoryBlock);
            this->m_memoryBlock = other.m_memoryBlock;
            this->m_totalSize = other.m_totalSize;
            this->m_offset = other.m_offset;

            // Nullify the source pointer to prevent double-free in the source's destructor.
            other.m_memoryBlock = nullptr;
            other.m_totalSize = 0;
            other.m_offset = 0;
        }

        return *this;
    }

     /**
     * @brief Allocates aligned memory from the arena
     * @param size Bytes to allocate
     * @param align Alignment (must be power of 2)
     * @return Allocated memory, or nullptr if out of space
     */
    [[nodiscard]] void* Alloc(const size_t size, const size_t align = alignof(max_align_t)) {
        const uintptr_t currentPtr = reinterpret_cast<uintptr_t>(m_memoryBlock) + m_offset;

        // Optimization: Using bitwise AND (&) for alignment calculation is significantly
        // faster than the modulo (%) operator. Requires 'align' to be a power of 2.
        uintptr_t offset = currentPtr & (align - 1);
        uintptr_t padding = (offset == 0) ? 0 : (align - offset);

        if (m_offset + padding + size > m_totalSize) {
            return nullptr;
        }

        const uintptr_t nextAddress = currentPtr + padding;
        m_offset += padding + size;
        return reinterpret_cast<void*>(nextAddress);
    }

    /** @brief Resets arena to empty state (does not call destructors) */
    void Reset() {
        m_offset = 0;
    }

    /**
    * @brief Constructs object in-place
    * @return Pointer to constructed object, or nullptr if out of space
    */
    template <typename T, typename... Args>
    [[nodiscard]] T* New(Args&&... args) {
        void* mem = Alloc(sizeof(T), alignof(T));
        if (!mem)
            return nullptr;

        return new (mem) T(std::forward<Args>(args)...);
    }

    /**
    * @brief Allocates array of objects (constructors NOT called)
    */
    template <typename T>
    [[nodiscard]] T* AllocArray(const size_t count) {
        return static_cast<T*>(Alloc(sizeof(T) * count, alignof(T)));
    }

    [[nodiscard]] size_t GetUsedMemory() const {
        return m_offset;
    }
    [[nodiscard]] size_t GetTotalSize() const {
        return m_totalSize;
    }
    [[nodiscard]] float GetUsageRatio() const {
        return static_cast<float>(m_offset) / static_cast<float>(m_totalSize);
    }

    /**
    * @brief Saves current position for partial reset
    * @see ResetToMarker()
    */
    using Marker = size_t;
    [[nodiscard]] Marker GetMarker() const {
        return m_offset;
    }

    /** @brief Resets arena to a previously saved marker */
    void ResetToMarker(const Marker marker) {
        m_offset = marker;
    }

private:
    std::byte* m_memoryBlock = nullptr; // Main memory buffer
    size_t m_totalSize = 0; // Total capacity
    size_t m_offset = 0; // Current allocation offset
};
#endif //ARENA_ALLOCATOR_H
