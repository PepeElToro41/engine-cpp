#pragma once

#include "dynamic_array.hpp"
#include "defines.hpp"
#include "core/memory/memory.hpp"
#include "platform/platform.hpp"


using SparseId = u64;
using SparseIdLow = u64;
using SparseGeneration = u64;

namespace SPARSE_LIST {
    constexpr u64 ELEMENT_BITS = 32;
    constexpr u64 ELEMENT_SIZE = 1ull << ELEMENT_BITS;
    constexpr u64 ELEMENT_MASK = ELEMENT_SIZE - 1;

    constexpr u64 GENERATION_BITS = 32;
    constexpr u64 GENERATION_SIZE = 1ull << GENERATION_BITS;

    constexpr usz PAGE_SIZE_BITS = 6; // 64 pages
    constexpr usz PAGE_SIZE = 1ull << PAGE_SIZE_BITS;
    constexpr usz PAGE_MASK = PAGE_SIZE - 1;

    inline SparseIdLow element_low(const SparseId id) {
        return id & ELEMENT_MASK;
    }
    inline SparseGeneration element_generation(const SparseId id) {
        return id >> ELEMENT_BITS;
    }
    inline SparseId append_generation(const SparseIdLow id, const SparseGeneration generation) {
        return id | (generation << ELEMENT_BITS);
    }
    inline usz get_page_index(const SparseId id) {
        return id >> PAGE_SIZE_BITS;
    }
    inline usz get_page_offset(const SparseId id) {
        return id & PAGE_MASK;
    }

    inline SparseId increment_generation(const SparseId element) {
        if (element > ELEMENT_SIZE) {
            const SparseIdLow id = element_low(element);
            const SparseGeneration new_generation = element_generation(element) + 1;

            if (new_generation <= GENERATION_SIZE) {
                return append_generation(id, new_generation);
            }
            return append_generation(id, 1);
        }
        return append_generation(element, 1);
    }
}


template <typename T>
struct SparsePage {
    T* data = nullptr;
    u64* dense = nullptr;

    void allocate(BaseAllocator* allocator) {
        this->data = MEMORY::alloc<T>(allocator, SPARSE_LIST::PAGE_SIZE);
        this->dense = MEMORY::alloc<u64>(allocator, SPARSE_LIST::PAGE_SIZE);
    }
    void free(BaseAllocator* allocator) {
        MEMORY::free(allocator, this->data);
        MEMORY::free(allocator, this->dense);
        this->data = nullptr;
        this->data = nullptr;
    }
};

template <typename T>
struct SparseList {
    BaseAllocator* allocator;
    DynamicArray<SparsePage<T>> sparse_pages;
    DynamicArray<u64> dense_list;

    u64 alive_count = 0;
    u64 next_element = 0;

    SparseList(BaseAllocator* allocator) :
        allocator(allocator),
        sparse_pages(allocator),
        dense_list(allocator) 
    {};

    SparsePage<T>* ensure_page(u64 page_index) {
        if (page_index < this->sparse_pages.size) {
            return this->sparse_pages.get_ref(page_index); 
        }

        const usz missing_pages = (page_index - this->sparse_pages.size) + 1;
        for (usz i = 0; i < missing_pages; i++) {
            SparsePage<T> empty_page{};
            empty_page.allocate(this->allocator);
            this->sparse_pages.push(empty_page);
        }

        return this->sparse_pages.get_ref(page_index);
    }

    T* new_element() {
        if (this->alive_count < this->dense_list.size - 1) {
            // recycling dead element
            usz dense = this->alive_count++;

            const SparseId element = this->dense_list.get(dense);
            const SparseIdLow element_id = SPARSE_LIST::element_low(element);

            const usz page_index = SPARSE_LIST::get_page_index(element_id);
            const usz page_offset = SPARSE_LIST::get_page_offset(element_id);

            SparsePage<T>* page = this->ensure_page(page_index);

            page->dense[page_offset] = dense;
            return page->data + page_offset;
        }

        const SparseId next_id = this->next_element++;
        this->dense_list.push(next_id);

        const usz dense = this->alive_count++;
        const usz page_index = SPARSE_LIST::get_page_index(next_id);
        const usz page_offset =  SPARSE_LIST::get_page_offset(next_id);

        SparsePage<T>* page = this->ensure_page(page_index);
        page->dense[page_offset] = dense;

        PLATFORM::memzero(&page->data[page_offset], sizeof(T));
        return &page->data[page_offset];
    }
    T* get_element(const SparseId id) const {
        const SparseIdLow id_low = SPARSE_LIST::element_low(id);
        const usz page_index = SPARSE_LIST::get_page_index(id_low);
        const usz page_offset = SPARSE_LIST::get_page_offset(id_low);

        if (page_index < this->sparse_pages.size) {
            return &this->sparse_pages.get(page_index).data[page_offset];
        }
        return nullptr;
    }
    SparseId get_latest_alive() const {
        const usz dense = this->alive_count;
        if (!dense) {
            return 0;
        }

        const SparseId last_alive = this->dense_list.get(dense);
        return last_alive;
    }

    void delete_element(SparseId sparse_id) {
        const SparseIdLow id = SPARSE_LIST::element_low(sparse_id);
        const usz page_index = SPARSE_LIST::get_page_index(id);
        const usz page_offset = SPARSE_LIST::get_page_offset(id);

        if (page_index >= this->sparse_pages.size) {
            return;
        }

        SparsePage<T> page = this->sparse_pages.get(page_index);
        const usz dense_index = page.dense[page_offset];
        const usz latest_alive_index = --this->alive_count;
        
        if(dense_index != latest_alive_index) {
            const SparseId latest_alive = this->dense_list.get(latest_alive_index);
            this->dense_list.set(dense_index, latest_alive);
            this->dense_list.set(latest_alive_index, SPARSE_LIST::increment_generation(id));
        
            SparsePage<T> latest_page = this->sparse_pages.get(SPARSE_LIST::get_page_index(latest_alive));
        
            latest_page.dense[SPARSE_LIST::get_page_offset(latest_alive)] = dense_index;
            page.dense[page_offset] = latest_alive_index;
        } else {
            this->dense_list.set(dense_index, SPARSE_LIST::increment_generation(id));
        }
    }

    void free() {
        for (SparsePage<T> page : this->sparse_pages) {
            page.free(this->allocator);
        }
        this->sparse_pages.free();
        this->dense_list.free();
    }
};