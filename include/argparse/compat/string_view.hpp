#pragma once

#include <argparse/compat/detect.hpp>

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
