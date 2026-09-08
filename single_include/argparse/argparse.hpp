// ============================================================================
// argparse: High-performance, multi-standard (C++11/17/20/23) argument parser
// https://github.com/modern-cpp/argparse
// Distributed under the MIT License.
// ============================================================================
#pragma once



// --- Begin: compat/detect.hpp ---
#if defined(_MSVC_LANG)
    #define ARGPARSE_CPLUSPLUS _MSVC_LANG
#else
    #define ARGPARSE_CPLUSPLUS __cplusplus
#endif

#if defined(__has_include)
    #if __has_include(<version>)
        #include <version>
    #endif
#endif

#if ARGPARSE_CPLUSPLUS >= 201402L
    #define ARGPARSE_CONSTEXPR14 constexpr
#else
    #define ARGPARSE_CONSTEXPR14
#endif

// Detect std::print / std::println (C++23)
#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
    #define ARGPARSE_HAS_STD_PRINT 1
#elif defined(__has_include)
    #if __has_include(<print>) && ARGPARSE_CPLUSPLUS >= 202302L
        #define ARGPARSE_HAS_STD_PRINT 1
    #else
        #define ARGPARSE_HAS_STD_PRINT 0
    #endif
#else
    #define ARGPARSE_HAS_STD_PRINT 0
#endif

// Detect std::expected (C++23)
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
    #define ARGPARSE_HAS_STD_EXPECTED 1
#else
    #define ARGPARSE_HAS_STD_EXPECTED 0
#endif

// Detect std::span (C++20)
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
    #define ARGPARSE_HAS_STD_SPAN 1
#elif defined(__has_include)
    #if __has_include(<span>) && ARGPARSE_CPLUSPLUS >= 202002L
        #define ARGPARSE_HAS_STD_SPAN 1
    #else
        #define ARGPARSE_HAS_STD_SPAN 0
    #endif
#else
    #define ARGPARSE_HAS_STD_SPAN 0
#endif

// Detect std::string_view (C++17)
#if defined(__cpp_lib_string_view) && __cpp_lib_string_view >= 201606L
    #define ARGPARSE_HAS_STD_STRING_VIEW 1
#elif defined(__has_include)
    #if __has_include(<string_view>) && ARGPARSE_CPLUSPLUS >= 201703L
        #define ARGPARSE_HAS_STD_STRING_VIEW 1
    #else
        #define ARGPARSE_HAS_STD_STRING_VIEW 0
    #endif
#else
    #define ARGPARSE_HAS_STD_STRING_VIEW 0
#endif

// Detect Concepts (C++20)
#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
    #define ARGPARSE_HAS_CONCEPTS 1
#else
    #define ARGPARSE_HAS_CONCEPTS 0
#endif

// Detect std::format (C++20)
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    #define ARGPARSE_HAS_STD_FORMAT 1
#elif defined(__has_include)
    #if __has_include(<format>) && ARGPARSE_CPLUSPLUS >= 202002L
        #define ARGPARSE_HAS_STD_FORMAT 1
    #else
        #define ARGPARSE_HAS_STD_FORMAT 0
    #endif
#else
    #define ARGPARSE_HAS_STD_FORMAT 0
#endif
// --- End: compat/detect.hpp ---


// --- Begin: compat/string_view.hpp ---
#if ARGPARSE_HAS_STD_STRING_VIEW
    #include <string_view>
    namespace argparse {
        namespace compat {
            using std::string_view;

            inline bool starts_with(string_view sv, string_view prefix) noexcept {
#if defined(__cpp_lib_starts_ends_with) && __cpp_lib_starts_ends_with >= 201711L
                return sv.starts_with(prefix);
#else
                return sv.size() >= prefix.size() && sv.substr(0, prefix.size()) == prefix;
#endif
            }

            inline bool starts_with(string_view sv, char c) noexcept {
#if defined(__cpp_lib_starts_ends_with) && __cpp_lib_starts_ends_with >= 201711L
                return sv.starts_with(c);
#else
                return !sv.empty() && sv.front() == c;
#endif
            }
        }
        using compat::string_view;
        using compat::starts_with;
    }
#else
    #include <algorithm>
    #include <cstddef>
    #include <cstring>
    #include <string>
    #include <stdexcept>
    #include <ostream>

    namespace argparse {
    namespace compat {

    class string_view {
    public:
        using size_type = std::size_t;
        static constexpr size_type npos = static_cast<size_type>(-1);

        constexpr string_view() noexcept : m_data(""), m_size(0) {}
        constexpr string_view(const char* str, size_type len) noexcept : m_data(str), m_size(len) {}
        string_view(const char* str) noexcept : m_data(str), m_size(str ? std::strlen(str) : 0) {}
        string_view(const std::string& str) noexcept : m_data(str.data()), m_size(str.size()) {}

        [[nodiscard]] constexpr const char* data() const noexcept { return m_data; }
        [[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
        [[nodiscard]] constexpr size_type length() const noexcept { return m_size; }
        [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }

        [[nodiscard]] constexpr const char& operator[](size_type idx) const noexcept { return m_data[idx]; }
        [[nodiscard]] constexpr const char& front() const noexcept { return m_data[0]; }
        [[nodiscard]] constexpr const char& back() const noexcept { return m_data[m_size - 1]; }

        [[nodiscard]] constexpr const char* begin() const noexcept { return m_data; }
        [[nodiscard]] constexpr const char* end() const noexcept { return m_data + m_size; }
        [[nodiscard]] constexpr const char* cbegin() const noexcept { return m_data; }
        [[nodiscard]] constexpr const char* cend() const noexcept { return m_data + m_size; }

        void remove_prefix(size_type n) noexcept {
            m_data += n;
            m_size -= n;
        }

        void remove_suffix(size_type n) noexcept {
            m_size -= n;
        }

        [[nodiscard]] ARGPARSE_CONSTEXPR14 bool starts_with(string_view sv) const noexcept {
            if (m_size < sv.m_size) return false;
            for (size_type i = 0; i < sv.m_size; ++i) {
                if (m_data[i] != sv.m_data[i]) return false;
            }
            return true;
        }

        [[nodiscard]] constexpr bool starts_with(char c) const noexcept {
            return !empty() && front() == c;
        }

        [[nodiscard]] ARGPARSE_CONSTEXPR14 string_view substr(size_type pos = 0, size_type count = npos) const {
            if (pos > m_size) throw std::out_of_range("string_view::substr out of range");
            size_type rcount = (count == npos || pos + count > m_size) ? (m_size - pos) : count;
            return string_view(m_data + pos, rcount);
        }

        [[nodiscard]] size_type find(char c, size_type pos = 0) const noexcept {
            for (size_type i = pos; i < m_size; ++i) {
                if (m_data[i] == c) return i;
            }
            return npos;
        }

        [[nodiscard]] size_type find_first_of(string_view s, size_type pos = 0) const noexcept {
            for (size_type i = pos; i < m_size; ++i) {
                for (size_type j = 0; j < s.m_size; ++j) {
                    if (m_data[i] == s.m_data[j]) return i;
                }
            }
            return npos;
        }

        [[nodiscard]] explicit operator std::string() const {
            return std::string(m_data, m_size);
        }

        [[nodiscard]] bool operator==(string_view other) const noexcept {
            if (m_size != other.m_size) return false;
            return std::memcmp(m_data, other.m_data, m_size) == 0;
        }

        [[nodiscard]] bool operator!=(string_view other) const noexcept {
            return !(*this == other);
        }

        [[nodiscard]] bool operator<(string_view other) const noexcept {
            int cmp = std::memcmp(m_data, other.m_data, std::min(m_size, other.m_size));
            if (cmp != 0) return cmp < 0;
            return m_size < other.m_size;
        }

    private:
        const char* m_data;
        size_type m_size;
    };

    inline bool starts_with(string_view sv, string_view prefix) noexcept {
        return sv.starts_with(prefix);
    }

    inline bool starts_with(string_view sv, char c) noexcept {
        return sv.starts_with(c);
    }

    inline std::ostream& operator<<(std::ostream& os, string_view sv) {
        return os.write(sv.data(), static_cast<std::streamsize>(sv.size()));
    }

    } // namespace compat
    using compat::string_view;
    using compat::starts_with;
    } // namespace argparse
#endif
// --- End: compat/string_view.hpp ---


// --- Begin: compat/span.hpp ---
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
// --- End: compat/span.hpp ---


// --- Begin: compat/expected.hpp ---
#if ARGPARSE_HAS_STD_EXPECTED
    #include <expected>
    namespace argparse {
        namespace compat {
            using std::expected;
            using std::unexpected;
            using std::unexpect;
            using std::unexpect_t;
        }
        using compat::expected;
        using compat::unexpected;
        using compat::unexpect;
        using compat::unexpect_t;
    }
#else
    #include <exception>
    #include <new>
    #include <stdexcept>
    #include <type_traits>
    #include <utility>

    namespace argparse {
    namespace compat {

    struct unexpect_t {
        explicit unexpect_t() = default;
    };
#if ARGPARSE_CPLUSPLUS >= 201703L
    inline constexpr unexpect_t unexpect{};
#else
    constexpr unexpect_t unexpect{};
#endif

    template <typename E>
    class unexpected {
    public:
        constexpr explicit unexpected(const E& err) : m_error(err) {}
        constexpr explicit unexpected(E&& err) : m_error(std::move(err)) {}

        [[nodiscard]] constexpr const E& error() const& noexcept { return m_error; }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 E& error() & noexcept { return m_error; }
        [[nodiscard]] constexpr const E&& error() const&& noexcept { return std::move(m_error); }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 E&& error() && noexcept { return std::move(m_error); }

    private:
        E m_error;
    };

#if ARGPARSE_CPLUSPLUS >= 201703L
    template <typename E>
    unexpected(E) -> unexpected<E>;
#endif

    template <typename T, typename E>
    class expected {
    public:
        using value_type = T;
        using error_type = E;
        using unexpected_type = unexpected<E>;

        expected() : m_has_value(true) {
            new (&m_val) T();
        }

        expected(const T& val) : m_has_value(true) {
            new (&m_val) T(val);
        }

        expected(T&& val) : m_has_value(true) {
            new (&m_val) T(std::move(val));
        }

        expected(const unexpected<E>& unex) : m_has_value(false) {
            new (&m_err) E(unex.error());
        }

        expected(unexpected<E>&& unex) : m_has_value(false) {
            new (&m_err) E(std::move(unex).error());
        }

        expected(const expected& other) : m_has_value(other.m_has_value) {
            if (m_has_value) {
                new (&m_val) T(other.value());
            } else {
                new (&m_err) E(other.error());
            }
        }

        expected(expected&& other) noexcept : m_has_value(other.m_has_value) {
            if (m_has_value) {
                new (&m_val) T(std::move(other).value());
            } else {
                new (&m_err) E(std::move(other).error());
            }
        }

        expected& operator=(const expected& other) {
            if (this != &other) {
                destroy();
                m_has_value = other.m_has_value;
                if (m_has_value) {
                    new (&m_val) T(other.value());
                } else {
                    new (&m_err) E(other.error());
                }
            }
            return *this;
        }

        expected& operator=(expected&& other) noexcept {
            if (this != &other) {
                destroy();
                m_has_value = other.m_has_value;
                if (m_has_value) {
                    new (&m_val) T(std::move(other).value());
                } else {
                    new (&m_err) E(std::move(other).error());
                }
            }
            return *this;
        }

        ~expected() {
            destroy();
        }

        [[nodiscard]] constexpr bool has_value() const noexcept { return m_has_value; }
        [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_has_value; }

        [[nodiscard]] constexpr const T& value() const& {
            if (!m_has_value) throw std::logic_error("Bad expected access");
            return *reinterpret_cast<const T*>(&m_val);
        }

        [[nodiscard]] ARGPARSE_CONSTEXPR14 T& value() & {
            if (!m_has_value) throw std::logic_error("Bad expected access");
            return *reinterpret_cast<T*>(&m_val);
        }

        [[nodiscard]] constexpr const T&& value() const&& {
            if (!m_has_value) throw std::logic_error("Bad expected access");
            return std::move(*reinterpret_cast<const T*>(&m_val));
        }

        [[nodiscard]] ARGPARSE_CONSTEXPR14 T&& value() && {
            if (!m_has_value) throw std::logic_error("Bad expected access");
            return std::move(*reinterpret_cast<T*>(&m_val));
        }

        [[nodiscard]] constexpr const T& operator*() const& noexcept { return *reinterpret_cast<const T*>(&m_val); }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 T& operator*() & noexcept { return *reinterpret_cast<T*>(&m_val); }
        [[nodiscard]] constexpr const T* operator->() const noexcept { return reinterpret_cast<const T*>(&m_val); }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 T* operator->() noexcept { return reinterpret_cast<T*>(&m_val); }

        template <typename U>
        [[nodiscard]] constexpr T value_or(U&& default_val) const& {
            return m_has_value ? value() : static_cast<T>(std::forward<U>(default_val));
        }

        template <typename U>
        [[nodiscard]] constexpr T value_or(U&& default_val) && {
            return m_has_value ? std::move(value()) : static_cast<T>(std::forward<U>(default_val));
        }

        [[nodiscard]] constexpr const E& error() const& noexcept { return *reinterpret_cast<const E*>(&m_err); }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 E& error() & noexcept { return *reinterpret_cast<E*>(&m_err); }
        [[nodiscard]] constexpr const E&& error() const&& noexcept { return std::move(*reinterpret_cast<const E*>(&m_err)); }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 E&& error() && noexcept { return std::move(*reinterpret_cast<E*>(&m_err)); }

        template <typename F>
        auto transform(F&& f) const& -> expected<typename std::remove_cv<decltype(f(std::declval<const T&>()))>::type, E> {
            using RetT = typename std::remove_cv<decltype(f(std::declval<const T&>()))>::type;
            if (m_has_value) {
                return expected<RetT, E>(f(value()));
            }
            return expected<RetT, E>(unexpected<E>(error()));
        }

        template <typename F>
        auto and_then(F&& f) const& -> decltype(f(std::declval<const T&>())) {
            if (m_has_value) {
                return f(value());
            }
            return unexpected<E>(error());
        }

    private:
        void destroy() noexcept {
            if (m_has_value) {
                reinterpret_cast<T*>(&m_val)->~T();
            } else {
                reinterpret_cast<E*>(&m_err)->~E();
            }
        }

        bool m_has_value;
        union {
            alignas(T) unsigned char m_val[sizeof(T)];
            alignas(E) unsigned char m_err[sizeof(E)];
        };
    };

    // Partial specialization for void value_type
    template <typename E>
    class expected<void, E> {
    public:
        using value_type = void;
        using error_type = E;
        using unexpected_type = unexpected<E>;

        constexpr expected() noexcept : m_has_value(true) {}

        expected(const unexpected<E>& unex) : m_has_value(false) {
            new (&m_err) E(unex.error());
        }

        expected(unexpected<E>&& unex) : m_has_value(false) {
            new (&m_err) E(std::move(unex).error());
        }

        expected(const expected& other) : m_has_value(other.m_has_value) {
            if (!m_has_value) {
                new (&m_err) E(other.error());
            }
        }

        expected(expected&& other) noexcept : m_has_value(other.m_has_value) {
            if (!m_has_value) {
                new (&m_err) E(std::move(other).error());
            }
        }

        expected& operator=(const expected& other) {
            if (this != &other) {
                destroy();
                m_has_value = other.m_has_value;
                if (!m_has_value) {
                    new (&m_err) E(other.error());
                }
            }
            return *this;
        }

        expected& operator=(expected&& other) noexcept {
            if (this != &other) {
                destroy();
                m_has_value = other.m_has_value;
                if (!m_has_value) {
                    new (&m_err) E(std::move(other).error());
                }
            }
            return *this;
        }

        ~expected() {
            destroy();
        }

        [[nodiscard]] constexpr bool has_value() const noexcept { return m_has_value; }
        [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_has_value; }

        void value() const {
            if (!m_has_value) throw std::logic_error("Bad expected access");
        }

        [[nodiscard]] constexpr const E& error() const& noexcept { return *reinterpret_cast<const E*>(&m_err); }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 E& error() & noexcept { return *reinterpret_cast<E*>(&m_err); }
        [[nodiscard]] constexpr const E&& error() const&& noexcept { return std::move(*reinterpret_cast<const E*>(&m_err)); }
        [[nodiscard]] ARGPARSE_CONSTEXPR14 E&& error() && noexcept { return std::move(*reinterpret_cast<E*>(&m_err)); }

    private:
        void destroy() noexcept {
            if (!m_has_value) {
                reinterpret_cast<E*>(&m_err)->~E();
            }
        }

        bool m_has_value;
        alignas(E) unsigned char m_err[sizeof(E)];
    };

    } // namespace compat
    using compat::expected;
    using compat::unexpected;
    using compat::unexpect;
    using compat::unexpect_t;
    } // namespace argparse
#endif
// --- End: compat/expected.hpp ---


// --- Begin: compat/traits.hpp ---
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace argparse {

template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <typename...>
using void_t = void;

struct parse_error;

template <typename T, typename = void>
struct value_parser;

#if ARGPARSE_HAS_CONCEPTS
    #include <concepts>

    template <typename T>
    concept parsable = requires(string_view sv) {
        { value_parser<T>::parse(sv) } -> std::same_as<expected<T, parse_error>>;
    };

    template <typename F, typename T>
    concept boolean_validator_for = requires(F&& f, const T& val) {
        { std::forward<F>(f)(val) } -> std::convertible_to<bool>;
    };

    template <typename F, typename T>
    concept result_validator_for = requires(F&& f, const T& val) {
        { std::forward<F>(f)(val) } -> std::same_as<expected<void, std::string>>;
    };

    template <typename F, typename T>
    concept validator_for = boolean_validator_for<F, T> || result_validator_for<F, T>;

    #define ARGPARSE_REQUIRES_PARSABLE(T) requires parsable<T>
    #define ARGPARSE_REQUIRES_PARSABLE_AND_VALIDATOR(T, F) requires parsable<T> && validator_for<F, T>
#else
    template <typename T, typename = void>
    struct is_parsable : std::false_type {};

    template <typename T>
    struct is_parsable<T, void_t<decltype(value_parser<T>::parse(std::declval<string_view>()))>> : std::true_type {};

#if ARGPARSE_CPLUSPLUS >= 201402L
    template <typename T>
    inline constexpr bool is_parsable_v = is_parsable<T>::value;
#endif

    #define ARGPARSE_REQUIRES_PARSABLE(T)
    #define ARGPARSE_REQUIRES_PARSABLE_AND_VALIDATOR(T, F)
#endif

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

#if ARGPARSE_CPLUSPLUS >= 201402L
template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;
#endif

} // namespace argparse
// --- End: compat/traits.hpp ---


// --- Begin: compat/print.hpp ---
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#if ARGPARSE_HAS_STD_PRINT
    #include <ostream>
    #include <print>

    namespace argparse {
    namespace compat {
        using std::print;
        using std::println;
    }
    using compat::print;
    using compat::println;
    }
#elif ARGPARSE_HAS_STD_FORMAT
    #include <format>
    #include <iostream>

    namespace argparse {
    namespace compat {

    template <typename... Args>
    inline void print(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << std::format(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void println(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
    }

    inline void println() {
        std::cout << '\n';
    }

    template <typename... Args>
    inline void print(FILE* f, std::format_string<Args...> fmt, Args&&... args) {
        std::string s = std::format(fmt, std::forward<Args>(args)...);
        std::fwrite(s.data(), 1, s.size(), f);
    }

    template <typename... Args>
    inline void println(FILE* f, std::format_string<Args...> fmt, Args&&... args) {
        std::string s = std::format(fmt, std::forward<Args>(args)...) + '\n';
        std::fwrite(s.data(), 1, s.size(), f);
    }

    inline void println(FILE* f) {
        std::fputc('\n', f);
    }

    template <typename... Args>
    inline void print(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
        os << std::format(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void println(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
        os << std::format(fmt, std::forward<Args>(args)...) << '\n';
    }

    inline void println(std::ostream& os) {
        os << '\n';
    }

    } // namespace compat
    using compat::print;
    using compat::println;
    } // namespace argparse
#else
    namespace argparse {
    namespace compat {

    template <typename T>
    inline void append_val(std::string& out, const T& val) {
        std::ostringstream oss;
        oss << val;
        out += oss.str();
    }

    inline void format_into(std::string& out, string_view fmt) {
        out.append(fmt.data(), fmt.size());
    }

    template <typename T, typename... Rest>
    inline void format_into(std::string& out, string_view fmt, const T& first, const Rest&... rest) {
        size_t pos = fmt.find('{');
        if (pos != string_view::npos && pos + 1 < fmt.size() && fmt[pos + 1] == '}') {
            out.append(fmt.data(), pos);
            append_val(out, first);
            format_into(out, fmt.substr(pos + 2), rest...);
        } else {
            out.append(fmt.data(), fmt.size());
        }
    }

    inline void print() {}
    inline void println() { std::cout << '\n'; }
    inline void println(std::ostream& os) { os << '\n'; }

    template <typename... Args>
    inline void print(string_view fmt, const Args&... args) {
        std::string s;
        format_into(s, fmt, args...);
        std::cout << s;
    }

    template <typename... Args>
    inline void println(string_view fmt, const Args&... args) {
        std::string s;
        format_into(s, fmt, args...);
        std::cout << s << '\n';
    }

    template <typename... Args>
    inline void print(std::ostream& os, string_view fmt, const Args&... args) {
        std::string s;
        format_into(s, fmt, args...);
        os << s;
    }

    template <typename... Args>
    inline void println(std::ostream& os, string_view fmt, const Args&... args) {
        std::string s;
        format_into(s, fmt, args...);
        os << s << '\n';
    }

    template <typename... Args>
    inline void print(FILE* f, string_view fmt, const Args&... args) {
        std::string s;
        format_into(s, fmt, args...);
        std::fwrite(s.data(), 1, s.size(), f);
    }

    template <typename... Args>
    inline void println(FILE* f, string_view fmt, const Args&... args) {
        std::string s;
        format_into(s, fmt, args...);
        s += '\n';
        std::fwrite(s.data(), 1, s.size(), f);
    }

    } // namespace compat
    using compat::print;
    using compat::println;
    } // namespace argparse
#endif
// --- End: compat/print.hpp ---


// --- Begin: core/error.hpp ---
#include <cstdint>
#include <sstream>
#include <string>

#if ARGPARSE_HAS_STD_FORMAT
    #include <format>
#endif

namespace argparse {

/**
 * @brief Categorized error codes returned by the argument parser.
 */
enum class error_code : uint8_t {
    success = 0,
    missing_required_argument,
    unexpected_positional,
    unknown_option,
    missing_value,
    invalid_value,
    mutually_exclusive_conflict,
    custom_validation_failed,
    choice_not_allowed
};

[[nodiscard]] inline ARGPARSE_CONSTEXPR14 string_view to_string_view(error_code code) noexcept {
    switch (code) {
        case error_code::success:
            return "success";
        case error_code::missing_required_argument:
            return "missing required argument";
        case error_code::unexpected_positional:
            return "unexpected positional argument";
        case error_code::unknown_option:
            return "unknown option";
        case error_code::missing_value:
            return "missing value for option";
        case error_code::invalid_value:
            return "invalid value conversion";
        case error_code::mutually_exclusive_conflict:
            return "mutually exclusive option conflict";
        case error_code::custom_validation_failed:
            return "custom validation constraint failed";
        case error_code::choice_not_allowed:
            return "value not in allowed choices";
    }
    return "unknown error";
}

/**
 * @brief Structured error information for parser diagnostic feedback.
 */
struct parse_error {
    error_code code{error_code::success};
    std::string argument_name{};
    std::string token{};
    std::string message{};

    constexpr parse_error() noexcept = default;

    parse_error(error_code c, string_view arg_name, string_view tok, std::string msg)
        : code(c), argument_name(arg_name.data(), arg_name.size()), token(tok.data(), tok.size()), message(std::move(msg)) {}

    parse_error(error_code c, string_view tok, std::string msg)
        : code(c), argument_name(""), token(tok.data(), tok.size()), message(std::move(msg)) {}

    [[nodiscard]] std::string to_string() const {
#if ARGPARSE_HAS_STD_FORMAT
        std::string result = std::format("Error [{}]: {}", std::string_view(to_string_view(code).data(), to_string_view(code).size()), message);
        if (!argument_name.empty()) {
            result += std::format(" (argument: '{}')", argument_name);
        }
        if (!token.empty()) {
            result += std::format(" (token: '{}')", token);
        }
        return result;
#else
        std::ostringstream oss;
        oss << "Error [" << to_string_view(code) << "]: " << message;
        if (!argument_name.empty()) {
            oss << " (argument: '" << argument_name << "')";
        }
        if (!token.empty()) {
            oss << " (token: '" << token << "')";
        }
        return oss.str();
#endif
    }

    [[nodiscard]] bool operator==(const parse_error& other) const noexcept {
        return code == other.code && argument_name == other.argument_name && token == other.token;
    }
};

} // namespace argparse
// --- End: core/error.hpp ---


// --- Begin: core/traits.hpp ---

// --- End: core/traits.hpp ---


// --- Begin: core/token.hpp ---
namespace argparse {

enum class token_type {
    long_option,       // e.g. --verbose or --port=8080
    short_option,      // e.g. -v or -p8080 or chained -xvf
    positional,        // e.g. filename.txt
    options_delimiter  // explicitly --
};

struct token {
    token_type type{token_type::positional};
    string_view raw{};
    string_view name{};
    bool has_inline_value{false};
    string_view inline_value{};

    constexpr token() noexcept = default;
    constexpr token(token_type t, string_view r, string_view n, bool has_val, string_view val) noexcept
        : type(t), raw(r), name(n), has_inline_value(has_val), inline_value(val) {}
};

} // namespace argparse
// --- End: core/token.hpp ---


// --- Begin: core/value_parser.hpp ---
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(__has_include)
    #if __has_include(<charconv>) && ARGPARSE_CPLUSPLUS >= 201703L
        #include <charconv>
        #define ARGPARSE_HAS_CHARCONV_HEADER 1
    #else
        #define ARGPARSE_HAS_CHARCONV_HEADER 0
    #endif
    #if __has_include(<filesystem>) && ARGPARSE_CPLUSPLUS >= 201703L
        #include <filesystem>
        #define ARGPARSE_HAS_STD_FILESYSTEM 1
    #else
        #define ARGPARSE_HAS_STD_FILESYSTEM 0
    #endif
#else
    #define ARGPARSE_HAS_CHARCONV_HEADER 0
    #define ARGPARSE_HAS_STD_FILESYSTEM 0
#endif

namespace argparse {

namespace detail {

[[nodiscard]] inline bool iequals(string_view lhs, string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) return false;
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

template <typename IntT>
inline IntT apply_sign(IntT val, bool negative, std::true_type) noexcept {
    return negative ? static_cast<IntT>(-val) : val;
}

template <typename IntT>
inline IntT apply_sign(IntT val, bool /*negative*/, std::false_type) noexcept {
    return val;
}

template <typename IntT>
[[nodiscard]] expected<IntT, parse_error> parse_integral(string_view sv) noexcept {
    if (sv.empty()) {
        return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Empty string cannot be parsed as integer"});
    }

    int base = 10;
    string_view num_part = sv;
    bool negative = false;

    if (starts_with(num_part, '-')) {
        if (std::is_unsigned<IntT>::value) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Cannot parse negative number into unsigned type"});
        }
        negative = true;
        num_part.remove_prefix(1);
    } else if (starts_with(num_part, '+')) {
        num_part.remove_prefix(1);
    }

    if (starts_with(num_part, "0x") || starts_with(num_part, "0X")) {
        base = 16;
        num_part.remove_prefix(2);
    } else if (starts_with(num_part, "0b") || starts_with(num_part, "0B")) {
        base = 2;
        num_part.remove_prefix(2);
    }

    if (num_part.empty()) {
        return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer format"});
    }

#if ARGPARSE_HAS_CHARCONV_HEADER
    IntT val{};
    auto [ptr, ec] = std::from_chars(num_part.data(), num_part.data() + num_part.size(), val, base);
    if (ec != std::errc{} || ptr != num_part.data() + num_part.size()) {
        if (ec == std::errc::result_out_of_range) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Integer value out of range"});
        }
        return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer literal"});
    }

    val = apply_sign(val, negative, std::is_signed<IntT>{});
    return val;
#else
    std::string s(num_part.data(), num_part.size());
    char* endptr = nullptr;
    if (std::is_signed<IntT>::value) {
        long long res = std::strtoll(s.c_str(), &endptr, base);
        if (endptr != s.c_str() + s.size()) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer literal"});
        }
        if (negative) res = -res;
        return static_cast<IntT>(res);
    } else {
        unsigned long long res = std::strtoull(s.c_str(), &endptr, base);
        if (endptr != s.c_str() + s.size()) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid integer literal"});
        }
        return static_cast<IntT>(res);
    }
#endif
}

} // namespace detail

// ==========================================
// Bool Parser
// ==========================================
template <>
struct value_parser<bool> {
    [[nodiscard]] static expected<bool, parse_error> parse(string_view sv) noexcept {
        if (detail::iequals(sv, "true") || detail::iequals(sv, "1") ||
            detail::iequals(sv, "yes") || detail::iequals(sv, "on") ||
            detail::iequals(sv, "t") || detail::iequals(sv, "y")) {
            return true;
        }
        if (detail::iequals(sv, "false") || detail::iequals(sv, "0") ||
            detail::iequals(sv, "no") || detail::iequals(sv, "off") ||
            detail::iequals(sv, "f") || detail::iequals(sv, "n")) {
            return false;
        }
        return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Cannot parse value as boolean"});
    }
};

// ==========================================
// Integer Parsers
// ==========================================
template <typename IntT>
struct value_parser<IntT, enable_if_t<std::is_integral<IntT>::value && !std::is_same<IntT, bool>::value>> {
    [[nodiscard]] static expected<IntT, parse_error> parse(string_view sv) noexcept {
        return detail::parse_integral<IntT>(sv);
    }
};

// ==========================================
// Floating-point Parsers
// ==========================================
template <typename FloatT>
struct value_parser<FloatT, enable_if_t<std::is_floating_point<FloatT>::value>> {
    [[nodiscard]] static expected<FloatT, parse_error> parse(string_view sv) noexcept {
        if (sv.empty()) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Empty string cannot be parsed as float"});
        }
#if ARGPARSE_HAS_CHARCONV_HEADER && defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L)
        FloatT val{};
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), val);
        if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid floating-point literal"});
        }
        return val;
#else
        std::string s(sv.data(), sv.size());
        char* endptr = nullptr;
        double val = std::strtod(s.c_str(), &endptr);
        if (endptr != s.c_str() + s.size()) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Invalid floating-point literal"});
        }
        return static_cast<FloatT>(val);
#endif
    }
};

// ==========================================
// String & String View Parsers
// ==========================================
template <>
struct value_parser<std::string> {
    [[nodiscard]] static expected<std::string, parse_error> parse(string_view sv) {
        return std::string(sv.data(), sv.size());
    }
};

template <>
struct value_parser<string_view> {
    [[nodiscard]] static expected<string_view, parse_error> parse(string_view sv) noexcept {
        return sv;
    }
};

// ==========================================
// Filesystem Path Parser
// ==========================================
#if ARGPARSE_HAS_STD_FILESYSTEM
template <>
struct value_parser<std::filesystem::path> {
    [[nodiscard]] static expected<std::filesystem::path, parse_error> parse(string_view sv) {
        return std::filesystem::path(std::string_view(sv.data(), sv.size()));
    }
};
#endif

// ==========================================
// Character Parser
// ==========================================
template <>
struct value_parser<char> {
    [[nodiscard]] static expected<char, parse_error> parse(string_view sv) noexcept {
        if (sv.size() != 1) {
            return unexpected<parse_error>(parse_error{error_code::invalid_value, sv, "Expected a single character"});
        }
        return sv.front();
    }
};

// ==========================================
// Vector Container Parser
// ==========================================
template <typename T>
struct value_parser<std::vector<T>> {
    [[nodiscard]] static expected<std::vector<T>, parse_error> parse(string_view sv) {
        std::vector<T> result;
        if (sv.empty()) {
            return result;
        }

        size_t start = 0;
        while (start < sv.size()) {
            size_t end = sv.find_first_of(",;", start);
            if (end == string_view::npos) {
                end = sv.size();
            }
            string_view part = sv.substr(start, end - start);
            while (!part.empty() && std::isspace(static_cast<unsigned char>(part.front()))) {
                part.remove_prefix(1);
            }
            while (!part.empty() && std::isspace(static_cast<unsigned char>(part.back()))) {
                part.remove_suffix(1);
            }

            if (!part.empty()) {
                auto parsed_item = value_parser<T>::parse(part);
                if (!parsed_item) {
                    return unexpected<parse_error>(parsed_item.error());
                }
                result.push_back(std::move(*parsed_item));
            }

            start = end + 1;
        }

        return result;
    }
};

} // namespace argparse
// --- End: core/value_parser.hpp ---


// --- Begin: config/action.hpp ---
#include <cstdint>

namespace argparse {

enum class action : uint8_t {
    store,        // Store single value (default for options that take arguments)
    store_true,   // Store boolean true if flag is present, false otherwise
    store_false,  // Store boolean false if flag is present, true otherwise
    append,       // Append each occurrence to a list/vector
    count         // Count number of times the flag appears (e.g. -vvv)
};

} // namespace argparse
// --- End: config/action.hpp ---


// --- Begin: config/argument.hpp ---
#include <cctype>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if ARGPARSE_HAS_STD_FORMAT
    #include <format>
#endif

namespace argparse {
namespace detail {
    inline std::string to_string_repr(const std::string& val) { return val; }
    inline std::string to_string_repr(string_view val) { return std::string(val.data(), val.size()); }
    inline std::string to_string_repr(const char* val) { return std::string(val ? val : ""); }
    inline std::string to_string_repr(bool val) { return val ? "true" : "false"; }

    template <typename T>
    inline std::string to_string_repr(const T& val) {
#if ARGPARSE_HAS_STD_FORMAT
        return std::format("{}", val);
#else
        std::ostringstream oss;
        oss << val;
        return oss.str();
#endif
    }
} // namespace detail

class argument {
public:
    using validator_fn = std::function<expected<void, std::string>(string_view)>;

    explicit argument(string_view name, string_view short_name = "")
        : m_name(name.data(), name.size()), m_short_name(short_name.data(), short_name.size()) {
        m_positional = !starts_with(name, '-');
        if (m_positional) {
            m_metavar = m_name;
        } else {
            string_view clean = name;
            while (starts_with(clean, '-')) {
                clean.remove_prefix(1);
            }
            m_metavar = std::string(clean.data(), clean.size());
            for (char& c : m_metavar) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                if (c == '-') c = '_';
            }
        }
    }

    argument& help(std::string text) {
        m_help = std::move(text);
        return *this;
    }

    argument& metavar(std::string mv) {
        m_metavar = std::move(mv);
        return *this;
    }

    argument& required(bool req = true) noexcept {
        m_required = req;
        return *this;
    }

    argument& flag() noexcept {
        m_action = action::store_true;
        m_has_default = true;
        m_default_value = "false";
        m_has_implicit = true;
        m_implicit_value = "true";
        return *this;
    }

    argument& store_false() noexcept {
        m_action = action::store_false;
        m_has_default = true;
        m_default_value = "true";
        m_has_implicit = true;
        m_implicit_value = "false";
        return *this;
    }

    argument& count() noexcept {
        m_action = action::count;
        m_has_default = true;
        m_default_value = "0";
        return *this;
    }

    argument& append() {
        m_action = action::append;
        return *this;
    }

    template <typename T>
    argument& default_value(const T& val) {
        m_has_default = true;
        m_default_value = detail::to_string_repr(val);
        return *this;
    }

    template <typename T>
    argument& implicit_value(const T& val) {
        m_has_implicit = true;
        m_implicit_value = detail::to_string_repr(val);
        return *this;
    }

    template <typename... Args>
    argument& choices(Args&&... chs) {
        std::vector<std::string> items = { std::string(chs)... };
        for (auto&& item : items) {
            m_choices.push_back(std::move(item));
        }
        return *this;
    }

    argument& choices(span<const string_view> chs) {
        for (size_t i = 0; i < chs.size(); ++i) {
            m_choices.emplace_back(chs[i].data(), chs[i].size());
        }
        return *this;
    }

    argument& choices(const std::vector<std::string>& chs) {
        m_choices = chs;
        return *this;
    }

    template <typename T, typename F>
    argument& validator(F func, std::string error_msg = "Constraint validation failed") {
        m_validators.push_back([func, error_msg](string_view raw) -> expected<void, std::string> {
            auto parsed = value_parser<T>::parse(raw);
            if (!parsed) {
                return unexpected<std::string>(parsed.error().message);
            }
            if (!func(*parsed)) {
                return unexpected<std::string>(error_msg);
            }
            return {};
        });
        return *this;
    }

    argument& group_id(size_t gid) noexcept {
        m_has_group = true;
        m_group_id = gid;
        return *this;
    }

    // Accessors
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const std::string& short_name() const noexcept { return m_short_name; }
    [[nodiscard]] const std::string& help() const noexcept { return m_help; }
    [[nodiscard]] const std::string& metavar() const noexcept { return m_metavar; }
    [[nodiscard]] action get_action() const noexcept { return m_action; }
    [[nodiscard]] bool is_required() const noexcept { return m_required; }
    [[nodiscard]] bool is_positional() const noexcept { return m_positional; }
    [[nodiscard]] bool is_flag() const noexcept { return m_action == action::store_true || m_action == action::store_false; }
    [[nodiscard]] bool takes_value() const noexcept { return m_action == action::store || m_action == action::append; }
    [[nodiscard]] bool has_default() const noexcept { return m_has_default; }
    [[nodiscard]] const std::string& default_value() const noexcept { return m_default_value; }
    [[nodiscard]] bool has_implicit() const noexcept { return m_has_implicit; }
    [[nodiscard]] const std::string& implicit_value() const noexcept { return m_implicit_value; }
    [[nodiscard]] const std::vector<std::string>& get_choices() const noexcept { return m_choices; }
    [[nodiscard]] const std::vector<validator_fn>& validators() const noexcept { return m_validators; }
    [[nodiscard]] bool has_group() const noexcept { return m_has_group; }
    [[nodiscard]] size_t get_group_id() const noexcept { return m_group_id; }

    [[nodiscard]] bool matches(string_view opt) const noexcept {
        return opt == string_view(m_name) || (!m_short_name.empty() && opt == string_view(m_short_name));
    }

private:
    std::string m_name;
    std::string m_short_name;
    std::string m_help;
    std::string m_metavar;
    action m_action{action::store};
    bool m_required{false};
    bool m_positional{false};
    bool m_has_default{false};
    std::string m_default_value;
    bool m_has_implicit{false};
    std::string m_implicit_value;
    std::vector<std::string> m_choices;
    std::vector<validator_fn> m_validators;
    bool m_has_group{false};
    size_t m_group_id{0};
};

} // namespace argparse
// --- End: config/argument.hpp ---


// --- Begin: config/argument_group.hpp ---
#include <cstddef>
#include <string>
#include <vector>

namespace argparse {

class argument_parser;

class argument_group {
public:
    argument_group(size_t id, argument_parser& parent, bool mutually_exclusive = false, bool required = false)
        : m_id(id), m_parent(&parent), m_mutually_exclusive(mutually_exclusive), m_required(required) {}

    argument& add_argument(string_view name, string_view short_name = "");

    [[nodiscard]] size_t id() const noexcept { return m_id; }
    [[nodiscard]] bool is_mutually_exclusive() const noexcept { return m_mutually_exclusive; }
    [[nodiscard]] bool is_required() const noexcept { return m_required; }
    [[nodiscard]] const std::vector<std::string>& argument_names() const noexcept { return m_argument_names; }

    void register_argument_name(string_view name) {
        m_argument_names.emplace_back(name.data(), name.size());
    }

private:
    size_t m_id;
    argument_parser* m_parent;
    bool m_mutually_exclusive{false};
    bool m_required{false};
    std::vector<std::string> m_argument_names;
};

} // namespace argparse
// --- End: config/argument_group.hpp ---


// --- Begin: engine/parse_result.hpp ---
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if ARGPARSE_HAS_STD_FORMAT
    #include <format>
#endif

namespace argparse {

namespace detail {
    template <typename T>
    struct try_get_helper {
        static expected<T, parse_error> parse(const std::vector<std::string>& vals, string_view) {
            return value_parser<T>::parse(string_view(vals.back()));
        }
    };

    template <typename T, typename Alloc>
    struct try_get_helper<std::vector<T, Alloc>> {
        using VecT = std::vector<T, Alloc>;
        static expected<VecT, parse_error> parse(const std::vector<std::string>& vals, string_view) {
            VecT vec;
            for (const auto& raw_val : vals) {
                auto parsed_sub = value_parser<VecT>::parse(string_view(raw_val));
                if (parsed_sub) {
                    for (auto&& item : *parsed_sub) {
                        vec.push_back(std::move(item));
                    }
                } else {
                    auto single_item = value_parser<T>::parse(string_view(raw_val));
                    if (single_item) {
                        vec.push_back(std::move(*single_item));
                    }
                }
            }
            return vec;
        }
    };
} // namespace detail

class parse_result {
public:
    parse_result() = default;

    [[nodiscard]] bool has(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end() || it->second.empty()) {
            return false;
        }
        if (it->second.size() == 1 && it->second.front() == "false") {
            return m_explicitly_present.count(std::string(canonical.data(), canonical.size())) > 0;
        }
        return true;
    }

    [[nodiscard]] bool is_explicit(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        return m_explicitly_present.count(std::string(canonical.data(), canonical.size())) > 0;
    }

    [[nodiscard]] size_t count(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end()) {
            return 0;
        }
        if (it->second.size() == 1) {
            auto parsed = value_parser<size_t>::parse(string_view(it->second.front()));
            if (parsed) {
                return *parsed;
            }
        }
        return it->second.size();
    }

    [[nodiscard]] bool has_raw(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        return it != m_values.end() && !it->second.empty();
    }

    [[nodiscard]] string_view get_raw(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end() || it->second.empty()) {
            return string_view{};
        }
        return string_view(it->second.back());
    }

    [[nodiscard]] std::vector<string_view> get_raw_list(string_view name) const {
        std::vector<string_view> result;
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it != m_values.end()) {
            result.reserve(it->second.size());
            for (const auto& s : it->second) {
                result.emplace_back(s.data(), s.size());
            }
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] expected<T, parse_error> try_get(string_view name) const noexcept {
        auto canonical = resolve_canonical_name(name);
        auto it = m_values.find(std::string(canonical.data(), canonical.size()));
        if (it == m_values.end() || it->second.empty()) {
            return unexpected<parse_error>(parse_error{error_code::missing_value, name, "", "Value not found"});
        }

        return detail::try_get_helper<T>::parse(it->second, name);
    }

    template <typename T>
    [[nodiscard]] T get(string_view name) const {
        auto val = try_get<T>(name);
        if (!val) {
#if ARGPARSE_HAS_STD_FORMAT
            throw std::runtime_error(std::format("Argument '{}' not found or failed to convert", std::string_view(name.data(), name.size())));
#else
            std::ostringstream oss;
            oss << "Argument '" << name << "' not found or failed to convert";
            throw std::runtime_error(oss.str());
#endif
        }
        return std::move(*val);
    }

    template <typename T>
    [[nodiscard]] T get_or(string_view name, T fallback) const noexcept {
        auto val = try_get<T>(name);
        if (val) {
            return std::move(*val);
        }
        return fallback;
    }

    [[nodiscard]] const std::vector<std::string>& positionals() const noexcept {
        return m_positionals;
    }

    void set_value(string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name.data(), canonical_name.size());
        m_values[name_str] = {std::move(value)};
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void append_value(string_view canonical_name, std::string value, bool is_explicit = true) {
        std::string name_str(canonical_name.data(), canonical_name.size());
        m_values[name_str].push_back(std::move(value));
        if (is_explicit) {
            m_explicitly_present.insert(name_str);
        }
    }

    void add_positional(std::string value) {
        m_positionals.push_back(std::move(value));
    }

    void register_alias(string_view alias, string_view canonical_name) {
        m_alias_map[std::string(alias.data(), alias.size())] = std::string(canonical_name.data(), canonical_name.size());
    }

private:
    [[nodiscard]] string_view resolve_canonical_name(string_view name) const noexcept {
        auto it = m_alias_map.find(std::string(name.data(), name.size()));
        if (it != m_alias_map.end()) {
            return string_view(it->second);
        }
        return name;
    }

    std::unordered_map<std::string, std::vector<std::string>> m_values;
    std::unordered_map<std::string, std::string> m_alias_map;
    std::unordered_set<std::string> m_explicitly_present;
    std::vector<std::string> m_positionals;
};

} // namespace argparse
// --- End: engine/parse_result.hpp ---


// --- Begin: engine/tokenizer.hpp ---
#include <cctype>

namespace argparse {

class tokenizer {
public:
    [[nodiscard]] static token tokenize(string_view arg) noexcept {
        if (arg == "--") {
            return token{
                token_type::options_delimiter,
                arg,
                arg,
                false,
                string_view{}
            };
        }

        if (starts_with(arg, "--") && arg.size() > 2) {
            auto eq_pos = arg.find('=');
            if (eq_pos != string_view::npos) {
                return token{
                    token_type::long_option,
                    arg,
                    arg.substr(0, eq_pos),
                    true,
                    arg.substr(eq_pos + 1)
                };
            }
            return token{
                token_type::long_option,
                arg,
                arg,
                false,
                string_view{}
            };
        }

        if (starts_with(arg, '-') && arg.size() > 1) {
            if (arg.size() > 1 && (std::isdigit(static_cast<unsigned char>(arg[1])))) {
                return token{
                    token_type::positional,
                    arg,
                    arg,
                    false,
                    string_view{}
                };
            }

            auto eq_pos = arg.find('=');
            if (eq_pos != string_view::npos) {
                return token{
                    token_type::short_option,
                    arg,
                    arg.substr(0, eq_pos),
                    true,
                    arg.substr(eq_pos + 1)
                };
            }

            return token{
                token_type::short_option,
                arg,
                arg,
                false,
                string_view{}
            };
        }

        return token{
            token_type::positional,
            arg,
            arg,
            false,
            string_view{}
        };
    }
};

} // namespace argparse
// --- End: engine/tokenizer.hpp ---


// --- Begin: engine/engine.hpp ---
#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if ARGPARSE_HAS_STD_FORMAT
    #include <format>
#endif

namespace argparse {

class engine {
public:
    [[nodiscard]] static expected<parse_result, parse_error> parse(
        const std::vector<argument>& arguments,
        const std::vector<argument_group>& groups,
        span<const string_view> raw_args) {

        parse_result result;

        std::unordered_map<std::string, const argument*> opt_map;
        std::vector<const argument*> positional_args;

        for (const auto& arg : arguments) {
            opt_map[arg.name()] = &arg;
            if (!arg.short_name().empty()) {
                opt_map[arg.short_name()] = &arg;
                result.register_alias(arg.short_name(), arg.name());
            }
            if (arg.is_positional()) {
                positional_args.push_back(&arg);
            }
        }

        std::unordered_map<size_t, std::string> group_first_seen_arg;
        size_t positional_idx = 0;
        bool treat_all_as_positional = false;
        size_t i = 0;

        auto check_and_record_group = [&](const argument* arg, string_view token_raw) -> expected<void, parse_error> {
            if (arg->has_group()) {
                size_t gid = arg->get_group_id();
                if (gid < groups.size() && groups[gid].is_mutually_exclusive()) {
                    auto it = group_first_seen_arg.find(gid);
                    if (it != group_first_seen_arg.end() && it->second != arg->name()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Argument '{}' conflicts with previously specified '{}'", arg->name(), it->second);
#else
                        std::string msg = "Argument '" + arg->name() + "' conflicts with previously specified '" + it->second + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::mutually_exclusive_conflict,
                            arg->name(),
                            token_raw,
                            std::move(msg)
                        });
                    }
                    group_first_seen_arg[gid] = arg->name();
                }
            }
            return {};
        };

        auto validate_and_store = [&](const argument* arg, string_view val) -> expected<void, parse_error> {
            if (!arg->get_choices().empty()) {
                bool found = false;
                for (const auto& choice : arg->get_choices()) {
                    if (string_view(choice) == val) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
#if ARGPARSE_HAS_STD_FORMAT
                    std::string msg = std::format("Value '{}' is not an allowed choice", std::string_view(val.data(), val.size()));
#else
                    std::string msg = "Value '" + std::string(val.data(), val.size()) + "' is not an allowed choice";
#endif
                    return unexpected<parse_error>(parse_error{
                        error_code::choice_not_allowed,
                        arg->name(),
                        val,
                        std::move(msg)
                    });
                }
            }

            for (const auto& v : arg->validators()) {
                auto v_res = v(val);
                if (!v_res) {
                    return unexpected<parse_error>(parse_error{
                        error_code::custom_validation_failed,
                        arg->name(),
                        val,
                        v_res.error()
                    });
                }
            }

            if (arg->get_action() == action::append) {
                result.append_value(arg->name(), std::string(val.data(), val.size()), true);
            } else {
                result.set_value(arg->name(), std::string(val.data(), val.size()), true);
            }
            return {};
        };

        while (i < raw_args.size()) {
            string_view current = raw_args[i];

            if (treat_all_as_positional) {
                if (positional_idx < positional_args.size()) {
                    const auto* p_arg = positional_args[positional_idx];
                    auto val_res = validate_and_store(p_arg, current);
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current.data(), current.size()));
                }
                i++;
                continue;
            }

            if (current == "--") {
                treat_all_as_positional = true;
                i++;
                continue;
            }

            token tok = tokenizer::tokenize(current);

            if (tok.type == token_type::positional) {
                if (positional_idx < positional_args.size()) {
                    const auto* p_arg = positional_args[positional_idx];
                    auto val_res = validate_and_store(p_arg, current);
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                    if (p_arg->get_action() != action::append) {
                        positional_idx++;
                    }
                } else {
                    result.add_positional(std::string(current.data(), current.size()));
                }
                i++;
            } else if (tok.type == token_type::long_option) {
                std::string tok_name(tok.name.data(), tok.name.size());
                auto it = opt_map.find(tok_name);
                if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                    std::string msg = std::format("Unknown option '{}'", tok_name);
#else
                    std::string msg = "Unknown option '" + tok_name + "'";
#endif
                    return unexpected<parse_error>(parse_error{
                        error_code::unknown_option,
                        tok.name,
                        tok.raw,
                        std::move(msg)
                    });
                }

                const argument* arg = it->second;
                auto grp_res = check_and_record_group(arg, tok.raw);
                if (!grp_res) return unexpected<parse_error>(grp_res.error());

                if (arg->is_flag()) {
                    if (tok.has_inline_value) {
                        auto val_res = validate_and_store(arg, tok.inline_value);
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                    } else {
                        std::string flag_val = arg->has_implicit() ? arg->implicit_value() :
                            (arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, string_view(flag_val));
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                    }
                    i++;
                } else if (arg->get_action() == action::count) {
                    size_t curr = result.count(arg->name());
                    result.set_value(arg->name(), std::to_string(curr + 1), true);
                    i++;
                } else {
                    string_view val;
                    if (tok.has_inline_value) {
                        val = tok.inline_value;
                        i++;
                    } else {
                        if (i + 1 >= raw_args.size()) {
#if ARGPARSE_HAS_STD_FORMAT
                            std::string msg = std::format("Option '{}' requires an argument", arg->name());
#else
                            std::string msg = "Option '" + arg->name() + "' requires an argument";
#endif
                            return unexpected<parse_error>(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                tok.raw,
                                std::move(msg)
                            });
                        }
                        val = raw_args[i + 1];
                        i += 2;
                    }
                    auto val_res = validate_and_store(arg, val);
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                }
            } else if (tok.type == token_type::short_option) {
                if (tok.has_inline_value) {
                    std::string tok_name(tok.name.data(), tok.name.size());
                    auto it = opt_map.find(tok_name);
                    if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Unknown option '{}'", tok_name);
#else
                        std::string msg = "Unknown option '" + tok_name + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::unknown_option,
                            tok.name,
                            tok.raw,
                            std::move(msg)
                        });
                    }
                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, tok.raw);
                    if (!grp_res) return unexpected<parse_error>(grp_res.error());

                    auto val_res = validate_and_store(arg, tok.inline_value);
                    if (!val_res) return unexpected<parse_error>(val_res.error());
                    i++;
                } else if (current.size() == 2) {
                    std::string cur_str(current.data(), current.size());
                    auto it = opt_map.find(cur_str);
                    if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Unknown option '{}'", cur_str);
#else
                        std::string msg = "Unknown option '" + cur_str + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::unknown_option,
                            current,
                            current,
                            std::move(msg)
                        });
                    }

                    const argument* arg = it->second;
                    auto grp_res = check_and_record_group(arg, current);
                    if (!grp_res) return unexpected<parse_error>(grp_res.error());

                    if (arg->is_flag()) {
                        std::string flag_val = arg->has_implicit() ? arg->implicit_value() :
                            (arg->get_action() == action::store_true ? "true" : "false");
                        auto val_res = validate_and_store(arg, string_view(flag_val));
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                        i++;
                    } else if (arg->get_action() == action::count) {
                        size_t curr = result.count(arg->name());
                        result.set_value(arg->name(), std::to_string(curr + 1), true);
                        i++;
                    } else {
                        if (i + 1 >= raw_args.size()) {
#if ARGPARSE_HAS_STD_FORMAT
                            std::string msg = std::format("Option '{}' requires an argument", arg->name());
#else
                            std::string msg = "Option '" + arg->name() + "' requires an argument";
#endif
                            return unexpected<parse_error>(parse_error{
                                error_code::missing_value,
                                arg->name(),
                                current,
                                std::move(msg)
                            });
                        }
                        string_view val = raw_args[i + 1];
                        auto val_res = validate_and_store(arg, val);
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                        i += 2;
                    }
                } else {
                    std::string first_opt = std::string("-") + current[1];
                    auto it = opt_map.find(first_opt);
                    if (it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                        std::string msg = std::format("Unknown option '{}'", first_opt);
#else
                        std::string msg = "Unknown option '" + first_opt + "'";
#endif
                        return unexpected<parse_error>(parse_error{
                            error_code::unknown_option,
                            current,
                            current,
                            std::move(msg)
                        });
                    }

                    const argument* first_arg = it->second;
                    if (first_arg->takes_value()) {
                        auto grp_res = check_and_record_group(first_arg, current);
                        if (!grp_res) return unexpected<parse_error>(grp_res.error());

                        string_view val = current.substr(2);
                        auto val_res = validate_and_store(first_arg, val);
                        if (!val_res) return unexpected<parse_error>(val_res.error());
                        i++;
                    } else {
                        for (size_t c = 1; c < current.size(); ++c) {
                            std::string opt_char = std::string("-") + current[c];
                            auto opt_it = opt_map.find(opt_char);
                            if (opt_it == opt_map.end()) {
#if ARGPARSE_HAS_STD_FORMAT
                                std::string msg = std::format("Unknown option '{}' in flag chain", opt_char);
#else
                                std::string msg = "Unknown option '" + opt_char + "' in flag chain";
#endif
                                return unexpected<parse_error>(parse_error{
                                    error_code::unknown_option,
                                    current,
                                    current,
                                    std::move(msg)
                                });
                            }

                            const argument* chained_arg = opt_it->second;
                            auto grp_res = check_and_record_group(chained_arg, current);
                            if (!grp_res) return unexpected<parse_error>(grp_res.error());

                            if (chained_arg->is_flag()) {
                                std::string flag_val = chained_arg->has_implicit() ? chained_arg->implicit_value() :
                                    (chained_arg->get_action() == action::store_true ? "true" : "false");
                                auto val_res = validate_and_store(chained_arg, string_view(flag_val));
                                if (!val_res) return unexpected<parse_error>(val_res.error());
                            } else if (chained_arg->get_action() == action::count) {
                                size_t curr = result.count(chained_arg->name());
                                result.set_value(chained_arg->name(), std::to_string(curr + 1), true);
                            } else if (chained_arg->takes_value()) {
                                string_view val;
                                if (c + 1 < current.size()) {
                                    val = current.substr(c + 1);
                                } else {
                                    if (i + 1 >= raw_args.size()) {
#if ARGPARSE_HAS_STD_FORMAT
                                        std::string msg = std::format("Option '{}' requires an argument", chained_arg->name());
#else
                                        std::string msg = "Option '" + chained_arg->name() + "' requires an argument";
#endif
                                        return unexpected<parse_error>(parse_error{
                                            error_code::missing_value,
                                            chained_arg->name(),
                                            current,
                                            std::move(msg)
                                        });
                                    }
                                    val = raw_args[i + 1];
                                    i++;
                                }
                                auto val_res = validate_and_store(chained_arg, val);
                                if (!val_res) return unexpected<parse_error>(val_res.error());
                                break;
                            }
                        }
                        i++;
                    }
                }
            }
        }

        for (const auto& arg : arguments) {
            if (!result.has(arg.name())) {
                if (arg.has_default()) {
                    result.set_value(arg.name(), arg.default_value(), false);
                }
            }
        }

        bool help_requested = result.has("--help") || result.has("-h");
        if (help_requested) {
            return result;
        }

        for (const auto& arg : arguments) {
            if (!result.has(arg.name()) && arg.is_required()) {
#if ARGPARSE_HAS_STD_FORMAT
                std::string msg = std::format("Required argument '{}' was not provided", arg.name());
#else
                std::string msg = "Required argument '" + arg.name() + "' was not provided";
#endif
                return unexpected<parse_error>(parse_error{
                    error_code::missing_required_argument,
                    arg.name(),
                    "",
                    std::move(msg)
                });
            }
        }

        for (const auto& group : groups) {
            if (group.is_mutually_exclusive() && group.is_required()) {
                if (group_first_seen_arg.find(group.id()) == group_first_seen_arg.end()) {
                    return unexpected<parse_error>(parse_error{
                        error_code::missing_required_argument,
                        "",
                        "",
                        "One of the mutually exclusive arguments must be provided"
                    });
                }
            }
        }

        return result;
    }
};

} // namespace argparse
// --- End: engine/engine.hpp ---


// --- Begin: format/formatter.hpp ---
#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace argparse {

class formatter {
public:
    [[nodiscard]] static std::string format_help(
        string_view program_name,
        string_view description,
        string_view epilog,
        const std::vector<argument>& arguments,
        [[maybe_unused]] const std::vector<argument_group>& groups) {

        std::ostringstream oss;

        oss << "Usage: " << program_name;

        bool has_options = false;
        std::vector<std::string> pos_usage;

        for (const auto& arg : arguments) {
            if (arg.is_positional()) {
                if (arg.is_required()) {
                    pos_usage.push_back("<" + arg.metavar() + ">");
                } else {
                    pos_usage.push_back("[<" + arg.metavar() + ">]");
                }
            } else {
                has_options = true;
            }
        }

        if (has_options) {
            oss << " [options]";
        }

        for (const auto& p : pos_usage) {
            oss << " " << p;
        }
        oss << "\n\n";

        if (!description.empty()) {
            oss << description << "\n\n";
        }

        std::vector<std::pair<std::string, std::string>> pos_entries;
        for (const auto& arg : arguments) {
            if (arg.is_positional()) {
                std::string label = arg.metavar();
                std::string desc = arg.help();
                if (arg.has_default()) {
                    desc += " (default: " + arg.default_value() + ")";
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size()) desc += ", ";
                    }
                    desc += "]";
                }
                pos_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!pos_entries.empty()) {
            oss << "Positional arguments:\n";
            size_t max_label_len = 0;
            for (const auto& entry : pos_entries) {
                max_label_len = std::max(max_label_len, entry.first.size());
            }

            for (const auto& entry : pos_entries) {
                oss << "  " << entry.first;
                if (entry.first.size() < max_label_len) {
                    oss << std::string(max_label_len - entry.first.size(), ' ');
                }
                oss << "    " << entry.second << "\n";
            }
            oss << "\n";
        }

        std::vector<std::pair<std::string, std::string>> opt_entries;
        for (const auto& arg : arguments) {
            if (!arg.is_positional()) {
                std::string label;
                if (!arg.short_name().empty()) {
                    label = arg.short_name();
                    if (arg.takes_value()) {
                        label += " <" + arg.metavar() + ">";
                    }
                    label += ", ";
                }
                label += arg.name();
                if (arg.takes_value()) {
                    label += " <" + arg.metavar() + ">";
                }

                std::string desc = arg.help();
                if (arg.is_required()) {
                    desc += " (required)";
                }
                if (arg.has_default()) {
                    desc += " (default: " + arg.default_value() + ")";
                }
                if (!arg.get_choices().empty()) {
                    desc += " [choices: ";
                    for (size_t i = 0; i < arg.get_choices().size(); ++i) {
                        desc += arg.get_choices()[i];
                        if (i + 1 < arg.get_choices().size()) desc += ", ";
                    }
                    desc += "]";
                }

                opt_entries.emplace_back(std::move(label), std::move(desc));
            }
        }

        if (!opt_entries.empty()) {
            oss << "Options:\n";
            size_t max_label_len = 0;
            for (const auto& entry : opt_entries) {
                max_label_len = std::max(max_label_len, entry.first.size());
            }

            for (const auto& entry : opt_entries) {
                oss << "  " << entry.first;
                if (entry.first.size() < max_label_len) {
                    oss << std::string(max_label_len - entry.first.size(), ' ');
                }
                oss << "    " << entry.second << "\n";
            }
            oss << "\n";
        }

        if (!epilog.empty()) {
            oss << epilog << "\n";
        }

        return oss.str();
    }

    [[nodiscard]] static std::string format_version(
        string_view program_name,
        string_view version) {
        std::ostringstream oss;
        oss << program_name << " " << version << "\n";
        return oss.str();
    }
};

} // namespace argparse
// --- End: format/formatter.hpp ---


// --- Begin: model/cli_model.hpp ---
#include <sstream>
#include <string>
#include <vector>

namespace argparse {

struct argument_model {
    std::string name;
    std::string short_name;
    std::string help;
    std::string metavar;
    action act{action::store};
    bool is_required{false};
    bool is_flag{false};
    bool is_positional{false};
    std::string default_value;
    std::string implicit_value;
    std::vector<std::string> choices;
};

struct cli_model {
    std::string program_name;
    std::string description;
    std::string epilog;
    std::string version{"1.0.0"};
    std::vector<argument_model> arguments;
    std::vector<std::vector<std::string>> mutually_exclusive_groups;

    [[nodiscard]] std::string to_json() const {
        std::ostringstream oss;
        auto escape_json = [](const std::string& s) -> std::string {
            std::string out;
            for (char c : s) {
                if (c == '"') out += "\\\"";
                else if (c == '\\') out += "\\\\";
                else if (c == '\n') out += "\\n";
                else if (c == '\r') out += "\\r";
                else if (c == '\t') out += "\\t";
                else out += c;
            }
            return out;
        };

        oss << "{\n";
        oss << "  \"program_name\": \"" << escape_json(program_name) << "\",\n";
        oss << "  \"description\": \"" << escape_json(description) << "\",\n";
        oss << "  \"version\": \"" << escape_json(version) << "\",\n";
        oss << "  \"arguments\": [\n";

        for (size_t i = 0; i < arguments.size(); ++i) {
            const auto& a = arguments[i];
            oss << "    {\n";
            oss << "      \"name\": \"" << escape_json(a.name) << "\",\n";
            oss << "      \"short_name\": \"" << escape_json(a.short_name) << "\",\n";
            oss << "      \"help\": \"" << escape_json(a.help) << "\",\n";
            oss << "      \"metavar\": \"" << escape_json(a.metavar) << "\",\n";
            oss << "      \"is_required\": " << (a.is_required ? "true" : "false") << ",\n";
            oss << "      \"is_flag\": " << (a.is_flag ? "true" : "false") << ",\n";
            oss << "      \"is_positional\": " << (a.is_positional ? "true" : "false") << ",\n";
            oss << "      \"default_value\": \"" << escape_json(a.default_value) << "\",\n";
            oss << "      \"implicit_value\": \"" << escape_json(a.implicit_value) << "\",\n";
            oss << "      \"choices\": [";
            for (size_t c = 0; c < a.choices.size(); ++c) {
                oss << "\"" << escape_json(a.choices[c]) << "\"";
                if (c + 1 < a.choices.size()) oss << ", ";
            }
            oss << "]\n";
            oss << "    }" << (i + 1 < arguments.size() ? "," : "") << "\n";
        }

        oss << "  ],\n";
        oss << "  \"mutually_exclusive_groups\": [\n";
        for (size_t g = 0; g < mutually_exclusive_groups.size(); ++g) {
            oss << "    [";
            for (size_t m = 0; m < mutually_exclusive_groups[g].size(); ++m) {
                oss << "\"" << escape_json(mutually_exclusive_groups[g][m]) << "\"";
                if (m + 1 < mutually_exclusive_groups[g].size()) oss << ", ";
            }
            oss << "]" << (g + 1 < mutually_exclusive_groups.size() ? "," : "") << "\n";
        }
        oss << "  ]\n";
        oss << "}\n";
        return oss.str();
    }
};

} // namespace argparse
// --- End: model/cli_model.hpp ---


// --- Begin: config/parser.hpp ---
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace argparse {

class argument_parser {
public:
    explicit argument_parser(string_view program_name, string_view description = "")
        : m_program_name(program_name.data(), program_name.size()),
          m_description(description.data(), description.size()) {
        add_argument("--help", "-h").help("Show this help message and exit").flag();
    }

    argument_parser& description(string_view desc) {
        m_description = std::string(desc.data(), desc.size());
        return *this;
    }

    argument_parser& epilog(string_view epi) {
        m_epilog = std::string(epi.data(), epi.size());
        return *this;
    }

    argument_parser& version(string_view ver) {
        m_version = std::string(ver.data(), ver.size());
        return *this;
    }

    argument& add_argument(string_view name, string_view short_name = "") {
        m_arguments.emplace_back(name, short_name);
        return m_arguments.back();
    }

    argument_group& add_mutually_exclusive_group(bool required = false) {
        size_t id = m_groups.size();
        m_groups.emplace_back(id, *this, true, required);
        return m_groups.back();
    }

    argument_group& add_group(bool required = false) {
        size_t id = m_groups.size();
        m_groups.emplace_back(id, *this, false, required);
        return m_groups.back();
    }

    [[nodiscard]] std::string format_help() const {
        return formatter::format_help(string_view(m_program_name), string_view(m_description),
                                     string_view(m_epilog), m_arguments, m_groups);
    }

    [[nodiscard]] std::string format_version() const {
        return formatter::format_version(string_view(m_program_name), string_view(m_version));
    }

    [[nodiscard]] cli_model export_model() const {
        cli_model model;
        model.program_name = m_program_name;
        model.description = m_description;
        model.epilog = m_epilog;
        model.version = m_version;

        for (const auto& arg : m_arguments) {
            argument_model am;
            am.name = arg.name();
            am.short_name = arg.short_name();
            am.help = arg.help();
            am.metavar = arg.metavar();
            am.act = arg.get_action();
            am.is_required = arg.is_required();
            am.is_flag = arg.is_flag();
            am.is_positional = arg.is_positional();
            am.default_value = arg.has_default() ? arg.default_value() : "";
            am.implicit_value = arg.has_implicit() ? arg.implicit_value() : "";
            am.choices = arg.get_choices();
            model.arguments.push_back(std::move(am));
        }

        for (const auto& grp : m_groups) {
            if (grp.is_mutually_exclusive()) {
                model.mutually_exclusive_groups.push_back(grp.argument_names());
            }
        }
        return model;
    }

    [[nodiscard]] std::string to_json() const {
        return export_model().to_json();
    }

    [[nodiscard]] expected<parse_result, parse_error>
    parse_args(span<const string_view> args) const noexcept {
        return engine::parse(m_arguments, m_groups, args);
    }

    [[nodiscard]] expected<parse_result, parse_error>
    parse_args(int argc, const char* const* argv) const {
        if (argc <= 1) {
            return parse_args(span<const string_view>{});
        }
        std::vector<string_view> views;
        views.reserve(static_cast<size_t>(argc - 1));
        for (int i = 1; i < argc; ++i) {
            views.emplace_back(argv[i]);
        }
        return parse_args(span<const string_view>(views));
    }

    [[nodiscard]] parse_result parse_or_throw(span<const string_view> args) const {
        auto res = parse_args(args);
        if (!res) {
            throw std::runtime_error(res.error().to_string());
        }
        return *res;
    }

    [[nodiscard]] parse_result parse_or_throw(int argc, const char* const* argv) const {
        auto res = parse_args(argc, argv);
        if (!res) {
            throw std::runtime_error(res.error().to_string());
        }
        return *res;
    }

    [[nodiscard]] parse_result parse_or_exit(int argc, const char* const* argv) const {
        auto res = parse_args(argc, argv);
        if (!res) {
            compat::println(std::cerr, "{}", res.error().to_string());
            compat::println(std::cerr);
            compat::print(std::cerr, "{}", format_help());
            std::exit(1);
        }
        if (res->has("--help") || res->has("-h")) {
            compat::print("{}", format_help());
            std::exit(0);
        }
        if (res->has("--version")) {
            compat::print("{}", format_version());
            std::exit(0);
        }
        return *res;
    }

    // Accessors
    [[nodiscard]] const std::string& program_name() const noexcept { return m_program_name; }
    [[nodiscard]] const std::string& get_description() const noexcept { return m_description; }
    [[nodiscard]] const std::string& get_version() const noexcept { return m_version; }
    [[nodiscard]] const std::vector<argument>& arguments() const noexcept { return m_arguments; }
    [[nodiscard]] const std::vector<argument_group>& groups() const noexcept { return m_groups; }

private:
    std::string m_program_name;
    std::string m_description;
    std::string m_epilog;
    std::string m_version{"1.0.0"};
    std::vector<argument> m_arguments;
    std::vector<argument_group> m_groups;
};

inline argument& argument_group::add_argument(string_view name, string_view short_name) {
    auto& arg = m_parent->add_argument(name, short_name);
    arg.group_id(m_id);
    register_argument_name(name);
    return arg;
}

} // namespace argparse
// --- End: config/parser.hpp ---
