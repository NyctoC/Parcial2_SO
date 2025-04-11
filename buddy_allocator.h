#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <cstddef>
#include <vector>
#include <cmath>
#include <iostream>

class BuddyAllocator {
public:
    BuddyAllocator(size_t total_size);
    ~BuddyAllocator();
    
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    size_t get_total_memory() const { return total_size; }
    size_t get_used_memory() const { return used_memory; }
    
private:
    struct Block {
        size_t size;
        bool free;
        void* memory;
        Block* buddy;
        Block* parent;
        
        Block(size_t s, void* mem) : size(s), free(true), memory(mem), buddy(nullptr), parent(nullptr) {}
    };
    
    size_t total_size;
    size_t used_memory;
    void* memory_pool;
    std::vector<Block*> free_blocks[32]; // Hasta bloques de 2^31
    
    size_t get_next_power_of_two(size_t size) const;
    size_t get_index_for_size(size_t size) const;
    size_t get_index(size_t size) const;  // Declaración añadida
    void split_block(Block* block);
    Block* find_best_block(size_t size);
    Block* get_buddy(Block* block);
    void merge_buddies(Block* block);
};

#endif