#include "heap_internal.h"
#include "heap_cpp.h"

#include <stdexcept>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

namespace heap {
    area heap;

    chunk* chunk::ptr2chunk(void* ptr) {
        return reinterpret_cast<chunk*>(ptr)-1;
    }

    void* chunk::chunk2ptr(chunk* chunk) {
        return reinterpret_cast<void*>(chunk+1);
    }

    inline void chunk::set_size(std::size_t nb) {
        this->size = nb;
    }

    std::size_t chunk::get_size() {
        return this->size;
    }

    void slot_chunk::set_size(std::size_t nb) {
        this->size = nb;
    }
    
    std::size_t slot_chunk::get_size() {
        return this->size;
    }

    ull large_chunk::get_index(std::size_t nb) {
        if(nb < 0x100)  return 0;
        if(nb < 0x200)  return 1;
        if(nb < 0x400)  return 2;
        if(nb < 0x800)  return 3;
        if(nb < 0x1000) return 4;
        if(nb < 0x2000) return 5;
        return 6;
    }

    void large_chunk::set_size(std::size_t nb) {
        this->size = nb;
    }

    std::size_t large_chunk::get_size() {
        return this->size;
    }

    large_chunk* large_chunk::get_next() {
        return this->next;
    }

    large_chunk* large_chunk::get_prev() {
        return this->prev;
    }

    void large_chunk::push_chunk(large_chunk* chunk) {
        chunk->prev = this->prev;
        chunk->next = this;
        this->prev->next = chunk;
        this->prev = chunk;
    }

    slot::slot()
    : freelist(nullptr) {
    }

    slots::slots()
    : count{0}, table{}  {
    }

    inline bool slots::is_in_slot(std::size_t size) {
        return size <= 0x100;
    }

    inline ull slots::get_slot_idx(std::size_t size) {
        return (size>>4)-1;
    }

    void slots::increase_count(ull idx) {
        set_count(idx, get_count(idx)+1);
    }

    void slots::decrease_count(ull idx) {
        set_count(idx, get_count(idx)-1);
    }

   
    int slots::get_count(ull idx) {
        return (count>>(idx*4))&0xf;
    }

    void slots::set_count(ull idx, unsigned char val) {
        count &= ~(0xFULL << (idx * 4));
        count |= ((ull)val << (idx * 4));
    }

    bool slots::is_full(ull idx) {
        return get_count(idx) == MAX_SLOT;
    }

    bool slots::is_empty(ull idx) {
        return get_count(idx) == 0;
    }

    void slots::push_slot_chunk(ull idx, slot_chunk* slot) {
        slot->next = table[idx].freelist;
        table[idx].freelist = slot;
        increase_count(idx);
    }

    inline slot_chunk* slots::pop_slot_chunk(ull idx) {
        slot_chunk* slot = table[idx].freelist;
        table[idx].freelist = slot->next;
        decrease_count(idx);
        return slot;
    }

    area::area()
    : slot_table(), blocks() {
        void* map = mmap(nullptr, 0x10000ull, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(map == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
        base_addr = reinterpret_cast<uintptr_t>(map);
        cursor = base_addr;
        end_addr = base_addr + 0x10000ull;
    }

    inline std::size_t area::align_page(std::size_t size) {
        return (size+0xfff)&~0xfff;
    }


    inline void* area::alloc(std::size_t size) {
        std::size_t nb = (size+0xf)&~0xf;
        if(nb < 0x10) {
            nb = 0x10;
        }

        if(slots::is_in_slot(nb)&&!slot_table.is_empty(slots::get_slot_idx(nb))) {
            ull slot_idx = slots::get_slot_idx(nb);
            slot_chunk* slot = slot_table.pop_slot_chunk(slot_idx);
            return chunk::chunk2ptr(slot);
        }

        for(ull block_index = large_chunk::get_index(nb); block_index < 7; block_index++) {
            large_chunk* block = blocks[block_index];
            if(block != nullptr) {
                do {
                    if(block->get_size() > nb && block->get_size() - nb >= 0x18) {
                        large_chunk* remain = reinterpret_cast<large_chunk*>(
                            reinterpret_cast<uintptr_t>(block) + nb + 0x8
                        );
                        remain->set_size(block->get_size() - nb - 0x10);
                        block->set_size(nb);
                        block->prev->next = block->next;
                        block->next->prev = block->prev;
                        ull remain_index = large_chunk::get_index(remain->get_size());
                        if(blocks[remain_index] == nullptr) {
                            blocks[remain_index] = remain;
                            remain->next = remain;
                            remain->prev = remain;
                        } else {
                            blocks[remain_index]->push_chunk(remain);
                        }
                    } else if(block->get_size() >= nb) {
                        block->prev->next = block->next;
                        block->next->prev = block->prev;
                    } else
                        continue;

                    if(blocks[block_index] == block) {
                        blocks[block_index] = block->next;
                        if(blocks[block_index] == block) {
                            blocks[block_index] = nullptr;
                        }
                    }
                    return chunk::chunk2ptr(block);
                } while((block = block->get_next()) != blocks[block_index]);
            }
        }
        

        if(cursor + nb + sizeof(chunk) > end_addr) {
            std::size_t map_size = extend_map(nb);
            if(map_size == 0) {
                map_size = nb <= 0x10000ull ? 0x10000ull : align_page(nb);

                void* map = mmap(nullptr, map_size, 
                    PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if(map == MAP_FAILED) {
                    throw std::runtime_error("mmap failed");
                }
                cursor = base_addr;
            }
            end_addr = base_addr + map_size;
        }

        
        chunk* chk = reinterpret_cast<chunk*>(cursor);
        chk->set_size(nb);
        cursor += nb + sizeof(chunk);
        return chunk::chunk2ptr(chk);
    }

    inline void area::free(void* ptr) {        
        chunk* chk = chunk::ptr2chunk(ptr);
        std::size_t size = chk->get_size();

        if(slots::is_in_slot(size)&&!slot_table.is_full(slots::get_slot_idx(size))) {
            ull slot_idx = slots::get_slot_idx(size);
            slot_chunk* slot = static_cast<slot_chunk*>(chk);            
            slot_table.push_slot_chunk(slot_idx, slot);
            return;
        }

        ull block_idx = large_chunk::get_index(size);
        large_chunk* block = static_cast<large_chunk*>(chk);
        if(blocks[block_idx] == nullptr) {
            blocks[block_idx] = block;
            block->next = block;
            block->prev = block;
        } else {
            blocks[block_idx]->push_chunk(block);
        }
    }

    inline void* area::realloc(void* ptr, std::size_t size) {
        chunk* chk = chunk::ptr2chunk(ptr);
        if(chk->get_size() >= size) {
            return ptr;
        }
        void* new_ptr = alloc(size);
        memcpy(new_ptr, ptr, size);
        free(ptr);

        return new_ptr;
    }
    
    inline std::size_t area::extend_map(std::size_t nb) {
        std::size_t prev_size = end_addr - base_addr;
        std::size_t next_size = prev_size + prev_size / 2;
        next_size = next_size - prev_size >= nb ? next_size : nb+prev_size;
        next_size = align_page(next_size);

        void* hint = reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(base_addr) + prev_size
        );

        void* result = mmap(hint, next_size - prev_size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (result == MAP_FAILED) {
            return 0;
        } else {
            return next_size;
        }
    }
}

void* heap_alloc(size_t size) {
    if(size == 0) {
        return nullptr;
    }
    return heap::heap.alloc(size);
}

void heap_free(void* ptr) {
    if(ptr == nullptr) {
        return;
    }
    heap::heap.free(ptr);
}

void* heap_realloc(void* ptr, size_t size) {
    if(size == 0) {
        heap::heap.free(ptr);
        return nullptr;
    }
    if(ptr == nullptr) {
        return heap::heap.alloc(size);
    }
    return heap::heap.realloc(ptr, size);
}