#pragma once

#include <argparse/compat/detect.hpp>

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
