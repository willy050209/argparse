#pragma once

#include <cstdint>

namespace argparse {

enum class action : uint8_t {
    store,       // Store single value (default for options that take arguments)
    store_true,  // Store boolean true if flag is present, false otherwise
    store_false, // Store boolean false if flag is present, true otherwise
    append,      // Append each occurrence to a list/vector
    count        // Count number of times the flag appears (e.g. -vvv)
};

} // namespace argparse
