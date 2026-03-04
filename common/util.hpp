#pragma once

#include <concepts>
#include <iterator>
#include <memory>
#include <utility>

#define __cpp_lib_expected 1

#include <cstdint>
#include <cxxabi.h>
#include <expected>
#include <system_error>
#include <iostream>
#include <format>

namespace UtilT {
    /**
     * @brief FNV-1a hash function for strings.
     */
    // Hash_t 通常是一个Uint64_t 或者 size_t, base 和 prime 是FNV-1a算法的常量 
    using Hash_t = std::uint64_t;
    inline constexpr Hash_t c_HashBase = 0xcbf29ce484222325ULL;
    inline constexpr Hash_t c_HashPrime = 0x100000001b3ULL;

    constexpr inline auto cHashString(std::string_view str, Hash_t base = c_HashBase, Hash_t prime = c_HashPrime) -> Hash_t {

        Hash_t hash_value = base;
        for(auto c : str)
            hash_value = (hash_value ^ static_cast<Hash_t>(c)) * prime;
        return hash_value;
    }


    /** @brief 因为gcc 13 无法导入print 所以自己实现一个print*/
    template<typename... Args>
    void println(std::format_string<Args...> fmt, Args&&... args){
        std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
    }



    template<auto c_X, auto... c_XS>
    inline constexpr bool c_IsOneofValues = ((c_X == c_XS) or ...);

    
    /** @brief 取得 Type Name 的字符串*/
    template<typename T>
    static std::string_view GetTypename() {
        static std::string s_TypeName = []() {
            int status = 0;
            char* demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
            std::string result = demangled ? demangled : typeid(T).name();
            if (demangled) std::free(demangled);
            return result;
        }();
        return s_TypeName;
    }

}














