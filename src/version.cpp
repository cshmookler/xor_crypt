// Local includes
#include "../src/version.hpp"

namespace xor_crypt {

const char* get_runtime_version() {
    return ::xor_crypt::compiletime_version;
}

} // namespace xor_crypt
