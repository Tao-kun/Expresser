#ifndef EXPRESSER_UTILS_HPP
#define EXPRESSER_UTILS_HPP

#include <cctype>

namespace expresser {
    // From https://en.cppreference.com/w/cpp/string/byte/isspace#Notes
#define IS_FUNC(f) \
    inline bool f(char ch){ \
        return std::f(static_cast<unsigned char>(ch)); \
    } \
    using __let_this_macro_end_with_a_simicolon_##f = int

    IS_FUNC(isprint);
    IS_FUNC(isspace);
    IS_FUNC(isblank);
    IS_FUNC(isalpha);
    IS_FUNC(isupper);
    IS_FUNC(islower);
    IS_FUNC(isdigit);
    IS_FUNC(isxdigit);
}

#endif //EXPRESSER_UTILS_HPP
