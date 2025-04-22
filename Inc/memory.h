
#ifndef MEMORY_H_
#define MEMORY_H_

struct FreeBlock;

struct Arena
{
    void *memory;
    size_t size;
    size_t offset;
    struct FreeBlock *free_list;
};

struct Arena *create_arena(size_t size, void *block);

void *arena_alloc(struct Arena *arena, size_t size);

void arena_dealloc(struct Arena *arena, void *ptr, size_t size);

void arena_reset(struct Arena *arena);

#endif
