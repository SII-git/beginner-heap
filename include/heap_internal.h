#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <tuple>


namespace heap {

#define INL __attribute__((always_inline))

typedef unsigned long long ull;
constexpr std::size_t MAX_SLOT = 0xf;


class chunk {
    public:
    static inline chunk* ptr2chunk(void* ptr);
    static inline void* chunk2ptr(chunk* chunk);
    inline void set_size(std::size_t nb);
    inline std::size_t get_size();
    protected:
    std::size_t size;

};

class slot_chunk: public chunk {
    public:

    inline void set_size(std::size_t nb);
    inline std::size_t get_size();
    friend class slots;
    private:
    slot_chunk* next;
};

class large_chunk: public chunk {
    friend class area;
    public:
    static inline ull get_index(std::size_t nb);


    inline void set_size(std::size_t nb);
    inline std::size_t get_size();

    inline large_chunk* get_next();
    inline large_chunk* get_prev();
    inline void push_chunk(large_chunk* chunk);
    private:
    large_chunk* next;
    large_chunk* prev;
};

class slot {
    public:
    slot();

    friend class slots;
    private:
    slot_chunk* freelist;
};  


class slots {
    public:
    slots();

    static inline bool is_in_slot(std::size_t size);

    static inline ull get_slot_idx(std::size_t size);
    

    inline void increase_count(ull idx);
    inline void decrease_count(ull idx);
    inline bool is_full(ull idx);
    inline bool is_empty(ull idx);
    inline void push_slot_chunk(ull idx, slot_chunk* slot);

    inline slot_chunk* pop_slot_chunk(ull idx);
    private:
    uint64_t count;
    std::array<slot, 16> table;

    inline int get_count(ull idx);
    inline void set_count(ull idx, unsigned char val);

};

class area {
    public:
    area();
    inline void* alloc(std::size_t size);
    inline void free(void* ptr);
    inline void* realloc(void* ptr, std::size_t size);
    
    static inline std::size_t align_page(std::size_t size);
    inline std::size_t extend_map(std::size_t nb);
    
    private:
    uintptr_t base_addr;
    uintptr_t cursor;
    uintptr_t end_addr;
    slots slot_table;
    std::array<large_chunk*,7> blocks;
};

} // heap