#include <chrono>
#include <iostream>
#include <string>
#include <iomanip>

#include "arena_allocator.h"

struct Particle {
    float x, y, z;
    int id;

    Particle(const float _x, const float _y, const float _z, const int _id)
        : x(_x), y(_y), z(_z), id(_id) {}
};

template <typename Func>
long long Measure(const std::string& name, Func func) {
    const auto start = std::chrono::high_resolution_clock::now();
    func();
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return duration;
}

int main() {
    constexpr int ITERATIONS = 1'000'000;
    constexpr int TEST_REPEATS = 5;

    std::cout << "--- BENCHMARK STARTING ---\n";
    std::cout << "Object Count: " << ITERATIONS << "\n";
    std::cout << "Object Size : " << sizeof(Particle) << " bytes\n\n";

    long long stdTotalTime = 0;
    for (int i = 0; i < TEST_REPEATS; i++) {
        stdTotalTime += Measure("Standard New/Delete", [] {
            // Fairness: We reserve vector capacity beforehand to exclude std::vector's
            // internal reallocation/copy costs from the benchmark. We want to measure raw 'new' vs 'Arena'.
            std::vector<Particle*> particles;
            particles.reserve(ITERATIONS);

            for (int j = 0; j < ITERATIONS; ++j) {
                particles.push_back(new Particle(1.0f, 2.0f, 3.0f, j));
            }

            for (auto* p : particles) {
                delete p;
            }
        });
    }

    double avgStdTime = static_cast<double>(stdTotalTime) / TEST_REPEATS;
    std::cout << "Standard Allocator Avg: " << avgStdTime << " ms\n";

    long long arenaTotalTime = 0;
    size_t arenaSize = ITERATIONS * (sizeof(Particle) + alignof(Particle)) + 1024;

    for (int i = 0; i < TEST_REPEATS; ++i) {
        arenaTotalTime += Measure("Arena Allocator", [arenaSize]() {
            ArenaAllocator arena(arenaSize);

            long long checkSum = 0;
            // Prevent Dead Code Elimination (DCE): We use the allocation result in a dummy calculation (checkSum).
            // Without this, the compiler might optimize away the entire loop in Release mode.
            for (int j = 0; j < ITERATIONS; ++j) {
                Particle* p = arena.New<Particle>(1.0f, 2.0f, 3.0f, j);
                checkSum += reinterpret_cast<uintptr_t>(p);
            }

            // Force the compiler to evaluate the checksum.
            volatile long long dummySink = checkSum;
            arena.Reset();
        });
    }
    double avgArenaTime = static_cast<double>(arenaTotalTime) / TEST_REPEATS;
    std::cout << "Arena Allocator Avg   : " << avgArenaTime << " ms\n";

    std::cout << "\n--- RESULTS ---\n";
    if (avgArenaTime > 0) {
        double speedup = avgStdTime / avgArenaTime;
        std::cout << "Speedup Factor: " << std::fixed << std::setprecision(2) << speedup << "x FASTER\n";
    } else {
        std::cout << "Arena was too fast to measure!\n";
    }

    return 0;
}