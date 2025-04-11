#include "buddy_allocator.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <list>
#include <unordered_map>

BuddyAllocator::BuddyAllocator(size_t size) : total_size(1), used_memory(0) {
    // Asegurar que es potencia de 2
    while (total_size < size) {
        total_size <<= 1;
    }
    
    memory_pool = malloc(total_size);
    if (!memory_pool) {
        throw std::bad_alloc();
    }
    
    Block* initial_block = new Block(total_size, memory_pool);
    free_blocks[get_index_for_size(total_size)].push_back(initial_block);
}

BuddyAllocator::~BuddyAllocator() {
    // Liberar todos los bloques y la memoria
    for (auto& list : free_blocks) {
        for (Block* block : list) {
            delete block;
        }
    }
    free(memory_pool);
}

void* BuddyAllocator::allocate(size_t size) {
    if (size == 0) return nullptr;
    
    // Ajustar el tamaño a la siguiente potencia de 2
    size_t adjusted_size = get_next_power_of_two(size);
    size_t index = get_index_for_size(adjusted_size);
    
    // Buscar un bloque libre del tamaño adecuado
    Block* block = find_best_block(adjusted_size);
    if (!block) {
        return nullptr; // No hay memoria disponible
    }
    
    // Dividir el bloque si es necesario
    while (block->size > adjusted_size) {
        split_block(block);
        block = find_best_block(adjusted_size);
    }
    
    block->free = false;
    used_memory += block->size;
    return block->memory;
}

void BuddyAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    
    // Buscar el bloque correspondiente al puntero
    Block* block_to_free = nullptr;
    for (auto& list : free_blocks) {
        for (Block* block : list) {
            if (block->memory <= ptr && static_cast<char*>(block->memory) + block->size > static_cast<char*>(ptr)) {
                block_to_free = block;
                break;
            }
        }
        if (block_to_free) break;
    }
    
    if (!block_to_free) {
        throw std::runtime_error("Intento de liberar memoria no asignada por el BuddyAllocator");
    }
    
    block_to_free->free = true;
    used_memory -= block_to_free->size;
    
    // Intentar fusionar con el buddy
    merge_buddies(block_to_free);
}

size_t BuddyAllocator::get_next_power_of_two(size_t size) const {
    if (size == 0) return 1;
    size_t power = 1;
    while (power < size) {
        power <<= 1;
    }
    return power;
}

size_t BuddyAllocator::get_index_for_size(size_t size) const {
    if (size == 0) return 0;
    return static_cast<size_t>(log2(size));
}

void BuddyAllocator::split_block(Block* block) {
    size_t new_size = block->size / 2;
    void* buddy_memory = static_cast<char*>(block->memory) + new_size;
    
    Block* buddy = new Block(new_size, buddy_memory);
    block->size = new_size;
    
    // Establecer la relación buddy
    block->buddy = buddy;
    buddy->buddy = block;
    
    // Establecer el padre
    buddy->parent = block->parent;
    block->parent = nullptr;
    
    // Actualizar las listas de bloques libres
    auto& list = free_blocks[get_index_for_size(block->size)];
    list.erase(std::remove(list.begin(), list.end(), block), list.end());
    
    free_blocks[get_index_for_size(new_size)].push_back(block);
    free_blocks[get_index_for_size(new_size)].push_back(buddy);
}

BuddyAllocator::Block* BuddyAllocator::find_best_block(size_t size) {
    size_t index = get_index_for_size(size);
    
    // Buscar en el índice correspondiente
    if (!free_blocks[index].empty()) {
        Block* block = free_blocks[index].back();
        free_blocks[index].pop_back();
        return block;
    }
    
    // Buscar en índices superiores
    for (size_t i = index + 1; i < 32; ++i) {
        if (!free_blocks[i].empty()) {
            Block* block = free_blocks[i].back();
            free_blocks[i].pop_back();
            return block;
        }
    }
    
    return nullptr; // No hay bloques disponibles
}

BuddyAllocator::Block* BuddyAllocator::get_buddy(Block* block) {
    if (!block || block->size == total_size) return nullptr;
    
    size_t buddy_offset = block->size;
    char* buddy_mem = static_cast<char*>(block->memory) + buddy_offset;
    
    // Verificar que el buddy está dentro del pool de memoria
    if (buddy_mem + buddy_offset > static_cast<char*>(memory_pool) + total_size) {
        return nullptr;
    }
    
    return block->buddy;
}

void BuddyAllocator::merge_buddies(Block* block) {
    if (!block) return;
    
    Block* buddy = get_buddy(block);
    if (!buddy || !buddy->free) return;
    
    // Verificar que ambos bloques estén en la lista libre
    auto& list = free_blocks[get_index_for_size(block->size)];
    auto block_it = std::find(list.begin(), list.end(), block);
    auto buddy_it = std::find(list.begin(), list.end(), buddy);
    
    if (block_it == list.end() || buddy_it == list.end()) return;
    
    // Eliminar ambos bloques
    list.erase(block_it);
    list.erase(buddy_it);
    
    // Crear bloque padre
    void* parent_mem = (block->memory < buddy->memory) ? block->memory : buddy->memory;
    Block* parent = new Block(block->size * 2, parent_mem);
    
    // Actualizar relaciones
    parent->buddy = nullptr;
    free_blocks[get_index_for_size(parent->size)].push_back(parent);
    
    // Liberar memoria
    delete block;
    delete buddy;
    
    // Fusionar recursivamente
    merge_buddies(parent);
}