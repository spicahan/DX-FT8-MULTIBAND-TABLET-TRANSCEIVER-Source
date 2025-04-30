

#include <stddef.h>

#include <stdint.h>
#include "memory.h"

struct FreeBlock
{
    size_t size;
    struct FreeBlock *next;
};

static uint8_t *__sbrk_heap_end = NULL;

void* _sbrk(ptrdiff_t incr)
{
  extern uint8_t _end; /* Symbol defined in the linker script */
  extern uint8_t _estack; /* Symbol defined in the linker script */
  extern uint32_t _Min_Stack_Size; /* Symbol defined in the linker script */
  const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
  const uint8_t *max_heap = (uint8_t *)stack_limit;
  uint8_t *prev_heap_end;

  /* Initialise heap end at first call */
  if (NULL == __sbrk_heap_end)
  {
    __sbrk_heap_end = &_end;
  }

  /* Protect heap from growing into the reserved MSP stack */
  if (__sbrk_heap_end + incr > max_heap)
  {
    return NULL;
  }

  prev_heap_end = __sbrk_heap_end;
  __sbrk_heap_end += incr;

  return (void *)prev_heap_end;
}

struct Arena *create_arena(size_t size, void *block)
{
    struct Arena *arena = block;
    if (arena != NULL)
    {
        arena->memory = arena + 1;
        arena->size = size;
        arena->offset = 0;
        arena->free_list = NULL;
    }
    return arena;
}

void *arena_alloc(struct Arena *arena, size_t size)
{
    struct FreeBlock **prev = &arena->free_list;
    struct FreeBlock *current = arena->free_list;
    struct FreeBlock **best_prev = NULL;
    struct FreeBlock *best_block = NULL;

    const size_t free_block_size = sizeof(struct FreeBlock);
    if (size < free_block_size)
        size = free_block_size;

    while (current != NULL)
    {
        if (current->size >= size && (!best_block || current->size < best_block->size))
        {
            best_prev = prev;
            best_block = current;
        }
        prev = &current->next;
        current = current->next;
    }

    if (best_block != NULL)
    {
        *best_prev = best_block->next;
        return best_block;
    }

    if (arena->offset + size > arena->size)
        return NULL;

    void *ptr = (unsigned char *)arena->memory + arena->offset;
    arena->offset += size;
    return ptr;
}

void arena_dealloc(struct Arena *arena, void *ptr, size_t size)
{
    struct FreeBlock *block = ptr;
    block->size = size;
    block->next = arena->free_list;
    arena->free_list = block;
}

void arena_reset(struct Arena *arena)
{
    if (arena != NULL)
    {
        arena->offset = 0;
        arena->free_list = NULL;
    }
}
