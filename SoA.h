#pragma once

#include <tuple>
#include <utility>
#include <cassert>

struct Allocator {
    virtual void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
    virtual void deallocate(void* p) = 0;
    virtual size_t allocation_size(void *p) = 0;
    
    template <typename T, typename... Args>
    T* make(Args&&... args) {
        return new(allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
    }
    
    template <typename T>
    void destroy(T* p) {
        if (p) {
            p->~T();
            deallocate(p);
        }
    }
};

struct Mallocator : public Allocator {
    void* allocate(size_t size, size_t alignment) override {
        (void)alignment;
        return malloc(size);
    }

    void deallocate(void* p) override {
        free(p);
    }

    size_t allocation_size(void* p) override { 
        // We aren't a real Allocator but who cares
        (void)p;
        return 0;
    }
};
    
template <int First, int Last, typename Lambda>
inline void static_for(Lambda const& f)
{
    if constexpr (First < Last)
    {
        f(std::integral_constant<int, First>{});
        static_for<First + 1, Last>(f);
    }
}
    
template <typename ...Elements>
struct SoA {
    
    static const size_t kNumArrays = sizeof...(Elements);
    
    template <size_t N>
    using NthTypeOf = typename std::tuple_element<N, std::tuple<Elements...>>::type;
    
    template <size_t Index>
    static auto get_array(void* data, size_t size) -> NthTypeOf<Index>* {
        if constexpr (Index == 0) {
            return reinterpret_cast<NthTypeOf<Index>*>(data);
        } else {
            auto prev_array = get_array<Index-1>(data, size);
            return reinterpret_cast<NthTypeOf<Index>*>(prev_array + size);
        }
    }
    
    SoA(Allocator* allocator) : m_allocator(allocator) {}
    
    ~SoA() {
        if (m_size > 0) {
            for (size_t i = 0; i < m_size; ++i) {
                static_for<0, kNumArrays>([&](auto array_i) {
                    using Type = NthTypeOf<array_i.value>;
                    get<array_i.value>(i).~Type();
                });
            }
            m_allocator->deallocate(m_data);
        }
    }
    
    void allocate(size_t size) {
        if (size == 0) {
            size = 32;
        }
        assert(size > m_size);
        assert(m_allocator);
    
        size_t old_size = m_size;
        void* old_data = m_data;
        
        const size_t bytes = size * sizeof(std::tuple<Elements...>);
        m_data = m_allocator->allocate(bytes);
        m_size = size;
        
        if (old_data) {
            static_for<0, kNumArrays>([&](auto i) {
                auto new_array = get_array<i.value>(m_data, m_size);
                auto old_array = get_array<i.value>(old_data, old_size);
                size_t bytes = m_n * sizeof(NthTypeOf<i.value>);
                memcpy(new_array, old_array, bytes);
            });
            m_allocator->deallocate(old_data);
        }
    }
    
    size_t add(Elements... args) {
        if (m_n == m_size) {
            allocate(m_size * 2);
        }
        
        size_t next = m_n;
        auto args_tuple = std::make_tuple(args...);
        static_for<0, kNumArrays>([&](auto i) {
            get<i.value>(next) = std::get<i.value>(args_tuple);
        });
        ++m_n;
        return next;
    }
    
    void remove(size_t index) {
        assert(index < m_n);
        size_t last = m_n - 1;
        auto key = get_array<0>(m_data, m_size)[index];
        auto last_key = get_array<0>(m_data, m_size)[last];
        
        static_for<0, kNumArrays>([&](auto i) {
            get<i.value>(index) = get<i.value>(last);
        });
    }
    
    template <size_t Index>
    auto get(size_t i) const -> NthTypeOf<Index>&
    {
        auto array = get_array<Index>(m_data, m_size);
        return array[i];
    }
    
    struct iterator {
        size_t i;
        
        iterator& operator++() {
            ++i;
            return *this;
        }
        
        bool operator!=(const iterator& other) {
            return i != other.i;
        }
        
        size_t operator*() {
            return i;
        }
    };
    
    iterator begin() {
        return { 0 };
    }
    
    iterator end() {
        return { m_n };
    }

private:
    size_t m_n = 0;
    size_t m_size = 0;
    Allocator* m_allocator = nullptr;
    void* m_data = nullptr;
};

