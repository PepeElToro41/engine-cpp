#include <cstddef>
#include <cstdint>

// TODO

struct TemporalArenaPage {
    TemporalArenaPage* previous;

    void* start;

    std::size_t size;
    std::size_t indent;

    char* data;
};

struct TemporalArena {

};