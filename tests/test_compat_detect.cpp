#include <argparse/compat/detect.hpp>

#if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L
static_assert(!ARGPARSE_HAS_STD_EXPECTED,
              "ARGPARSE_HAS_STD_EXPECTED must stay disabled when std::expected is not fully available.");
#else
static_assert(ARGPARSE_HAS_STD_EXPECTED,
              "ARGPARSE_HAS_STD_EXPECTED should be enabled when std::expected is available.");
#endif

int main() {
    return 0;
}
