#ifndef UTILS_H
#define UTILS_H

#include <vector_types.h>
#include <string>
namespace utils {
    bool dim3_compare(const dim3& a, const dim3& b);    
    bool dim3_equal(const dim3& a, const dim3& b);
    std::string dim3_to_string(const dim3& d);
}
#endif // !UTILS_H