#pragma once

#include <argparse/compat/detect.hpp>

#if ARGPARSE_HAS_STD_SPAN
    #include <span>
    namespace argparse {
        namespace compat {
            using std::span;
        }
        using compat::span;
    }
#else
    #include <type_traits>
    #include <vector>

    namespace argparse {
    namespace compat {

    template <typename T>
    class span {
    public:
        using element_type = T;
        using value_type = typename std::remove_cv<T>::type;
        using size_type = std::size_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using iterator = T*;
        using const_iterator = const T*;

        constexpr span() noexcept : m_data(nullptr), m_size(0) {}
        constexpr span(pointer ptr, size_type count) noexcept : m_data(ptr), m_size(count) {}
        constexpr span(pointer first, pointer last) noexcept : m_data(first), m_size(static_cast<size_type>(last - first)) {}

        template <typename U, typename Alloc,
                  typename = typename std::enable_if<std::is_convertible<U*, pointer>::value>::type>
        span(std::vector<U, Alloc>& vec) noexcept : m_data(vec.data()), m_size(vec.size()) {}

        template <typename U, typename Alloc,
                  typename = typename std::enable_if<std::is_convertible<const U*, pointer>::value>::type>
        span(const std::vector<U, Alloc>& vec) noexcept : m_data(vec.data()), m_size(vec.size()) {}

        [[nodiscard]] constexpr pointer data() const noexcept { return m_data; }
        [[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
        [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }

        [[nodiscard]] constexpr reference operator[](size_type idx) const noexcept { return m_data[idx]; }
        [[nodiscard]] constexpr reference front() const noexcept { return m_data[0]; }
        [[nodiscard]] constexpr reference back() const noexcept { return m_data[m_size - 1]; }

        [[nodiscard]] constexpr iterator begin() const noexcept { return m_data; }
        [[nodiscard]] constexpr iterator end() const noexcept { return m_data + m_size; }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return m_data; }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return m_data + m_size; }

    private:
        pointer m_data;
        size_type m_size;
    };

    } // namespace compat
    using compat::span;
    } // namespace argparse
#endif
