#include <vector>
#include <optional>

using SparseId = std::uint64_t;
using SparseIdLow = std::uint64_t;

namespace sparse_list {
    constexpr std::size_t ELEMENT_BITS = 10;
    constexpr std::size_t ELEMENT_SIZE = 1ull << ELEMENT_BITS;
    constexpr std::size_t ELEMENT_MASK = ELEMENT_SIZE - 1;
        
    constexpr std::size_t GENERATION_BITS = 16;
    constexpr std::size_t GENERATION_SIZE = 1ull << GENERATION_BITS;
    constexpr std::size_t GENERATION_MASK = GENERATION_SIZE - 1;
        
    constexpr std::size_t SPARSE_PAGE_BITS = 6;
    constexpr std::size_t SPARSE_PAGE_SIZE = 1ull << SPARSE_PAGE_BITS;
    constexpr std::size_t SPARSE_PAGE_MASK = SPARSE_PAGE_SIZE - 1;

    inline SparseIdLow sparse_id_low(SparseId id) {
        return id & ELEMENT_MASK;
    };
    inline std::size_t sparse_generation(SparseId id) {
        return id >> ELEMENT_BITS;
    };
    inline SparseId append_generation(SparseIdLow id, std::size_t generation) {
        return id | (generation << ELEMENT_BITS);
    };
    inline std::size_t get_page_index(SparseId id) {
        return id >> SPARSE_PAGE_BITS;
    }
    inline std::size_t get_page_offset(SparseId id) {
        return id & SPARSE_PAGE_MASK;
    };

    SparseId increment_generation(SparseId element) {
        if (element > ELEMENT_MASK) {
            SparseIdLow id = sparse_id_low(element);
            std::size_t new_generation = sparse_generation(element) + 1;
            if (new_generation >= GENERATION_SIZE) {
                return append_generation(id, 1);
            }

            return append_generation(id, new_generation);
        }
        return append_generation(element, 1);
    };
}

    
template <typename T>
struct SparsePage {
    T dense[sparse_list::SPARSE_PAGE_SIZE];
    T data[sparse_list::SPARSE_PAGE_SIZE];

    bool is_empty() const {
        return this->dense == nullptr && this->data == nullptr;
    }

    static SparsePage<T> alloc_page() {
        std::size_t* dense = new std::size_t[sparse_list::SPARSE_PAGE_SIZE];
        T* data = new T[sparse_list::SPARSE_PAGE_SIZE];

        SparsePage<T> page = {
            .dense = dense,
            .data = data,
        };

        return page;
    };

    static SparsePage get_empty_page() {
        SparsePage<T> page = {
            .dense = nullptr,
            .data = nullptr,
        };

        return page;
    };
};


template <typename T>
struct SparseList {
    std::vector<SparsePage<T>> sparse_pages;
    std::vector<SparseId> dense_list;

    std::size_t alive_count;
    SparseId next_element;

    SparsePage<T> ensure_page(std::size_t page_index) {
        if (page_index < this->sparse_pages.size()) {
            SparsePage<T> page = this->sparse_pages[page_index];

            if (page.is_empty()) {
                SparsePage<T> new_page = SparsePage<T>::alloc_page();
                this->sparse_pages[page_index] = new_page;
                return new_page;
            }

            return page;
        }

        std::size_t missing_pages = page_index - this->sparse_pages.size();

        for (std::size_t i = 0; i < missing_pages; i++) {
            // intermediate pages are lazily allocated
            SparsePage<T> empty_page = SparsePage<T>::get_empty_page();
            this->sparse_pages.push_back(empty_page);
        }

        SparsePage<T> page = SparsePage<T>::alloc_page();
        this->sparse_pages.push_back(page);

        return page;
    };

    std::optional<SparsePage<T>> get_valid_page(std::size_t page_index) const {
        if (page_index < this->sparse_pages.size()) {
            SparsePage<T> page = this->sparse_pages[page_index];
            if (page.is_empty()) {
                return {};
            }
            return page;
        }
        return {};
    }

    T* new_element(T value) {
        if(this->alive_count < this->dense_list.size() - 1) {
            // recycling dead element
            this->alive_count++;

            std::size_t dense = this->alive_count;

            SparseId element = this->dense_list[dense];
            SparseIdLow element_id = sparse_list::sparse_id_low(element);

            std::size_t page_index = sparse_list::get_page_index(element_id);
            std::size_t page_offset = sparse_list::get_page_offset(element_id);

            SparsePage<T> page = this->ensure_page(page_index);

            page.data[page_offset] = value; // this will copy it to the heap allocated list
            page.dense[page_offset] = dense;

            return &page.data[page_offset];
        }

        SparseId next_element = this->next_element++;
        this->dense_list.push_back(next_element);
        this->alive_count++;

        std::size_t dense = this->alive_count;
        std::size_t page_index = sparse_list::get_page_index(next_element);
        std::size_t page_offset = sparse_list::get_page_offset(next_element);

        SparsePage<T> page = this->ensure_page(page_index);

        page.data[page_offset] = value; // this will copy it to the heap allocated list
        page.dense[page_offset] = dense;

        return &page.data[page_offset];
    };
    SparseId get_latest_alive() const {
        std::size_t dense = this->alive_count;
        if (!dense) {
            return 0;
        }

        std::size_t last_alive = this->dense_list[dense];
        return last_alive;
    };
};