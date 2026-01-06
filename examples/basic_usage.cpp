#include <iostream>
#include "arena_allocator.h"

struct Player {
    int hp, xp, level;
    Player(int h, int x, int l) : hp(h), xp(x), level(l) {}
};

struct Enemy {
    int hp, damage;
    Enemy(int h, int d) : hp(h), damage(d) {}
};
int main() {
    std::cout << "--- Arena Allocator Demo ---\n";

    // Create arena (1KB)
    ArenaAllocator arena = ArenaAllocator(1024);

    // Create objects
    Player* p1 = arena.New<Player>(100, 50, 1);
    Enemy* e1  = arena.New<Enemy>(50, 10);
    Enemy* e2  = arena.New<Enemy>(60, 12);

    // "Demonstrating Cache Locality:
    // Since allocations are sequential, these addresses should be very close to each other.
    // This reduces cache misses compared to standard heap allocations which might be fragmented."

    std::cout << "Player Address: " << p1 << "\n";
    std::cout << "Enemy1 Address: " << e1 << " (Should be close to Player)\n";
    std::cout << "Enemy2 Address: " << e2 << " (Should be close to Enemy1)\n";

    // Check data integrity
    std::cout << "Player Level: " << p1->level << "\n";

    // Reset
    arena.Reset();
    std::cout << "Arena reset. Memory is ready to be overwritten.\n";

    return 0;
}
