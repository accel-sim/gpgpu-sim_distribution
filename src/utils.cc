#include "utils.h"

namespace utils {
    bool dim3_compare(const dim3& a, const dim3& b) {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }

    bool dim3_equal(const dim3& a, const dim3& b) {
        return !dim3_compare(a, b) && !dim3_compare(b, a);
    }

    std::string dim3_to_string(const dim3& d) {
        return std::to_string(d.x) + " " + std::to_string(d.y) + " " + std::to_string(d.z);
    }
}
