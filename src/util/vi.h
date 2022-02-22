#ifndef VI_H
#define VI_H

#if (defined(__GNUC__) && (__GNUC__ < 11)) || !defined(__cpp_concepts)
#define OLD_GCC
#endif

#include <vector>
#include <charconv>
#include <concepts>
#include <string_view>
#include <cstddef>
#include <system_error>
#include <optional>

namespace vi {

// return iterator instead of position
[[nodiscard]]
constexpr auto find_str_in_str(
        const std::string_view str,
        const std::string_view delim)
{
    auto ans{str.find(delim)};
    if(ans == std::string_view::npos)
        return str.end();
    return str.begin() + ans;
}

#ifdef OLD_GCC
template<typename T>
constexpr std::string_view make_sv_it(T begin, T end){
    // not doing &*begin seems to work
    return std::string_view{begin, std::size_t(std::distance(begin, end))};
}
#endif


#ifdef OLD_GCC
template<typename T>
#endif
constexpr void for_each_split_str(
        std::string_view str,
        const std::string_view delim,
#ifdef OLD_GCC
        const T& func)
#else
        const std::invocable<std::string_view> auto& func)
#endif
{
    if(delim == ""){
#ifdef OLD_GCC
        func(make_sv_it(str.begin(), str.end()));
#else
        func(std::string_view{str.begin(), str.end()});
#endif
        return;
    }
    auto i{str.begin()};
    auto j{find_str_in_str(str, delim)};
    while(j != str.end()){
#ifdef OLD_GCC
        func(make_sv_it(i, j));
#else
        func(std::string_view{i, j});
#endif
        i = j + delim.size();
#ifdef OLD_GCC
        str = {make_sv_it(i, str.end())};
#else
        str = {i, str.end()};
#endif
        j = {find_str_in_str(str, delim)};
    }
#ifdef OLD_GCC
    func(make_sv_it(i, str.end()));
#else
    func(std::string_view{i, str.end()});
#endif
}

// returns 0 for empty delimiter
[[nodiscard]]
constexpr std::size_t count_str_in_str(
        const std::string_view str,
        const std::string_view delim)
{
    std::size_t count{0};
    for_each_split_str(str, delim, [&count]([[maybe_unused]] auto s){
        ++count;
    });
    return count - 1;
}

[[nodiscard]]
inline auto split_str(
        const std::string_view str,
        const std::string_view delim)
{
    std::vector<std::string_view> v;
    for_each_split_str(str, delim, [&v](auto s){
        v.push_back(s);
    });
    return v;
}

// only fundamental types compatible with std::from_chars
template<typename T>
#ifndef OLD_GCC
concept from_chars_type =
    std::same_as<T, char> ||
    std::same_as<T, short> ||
    std::same_as<T, unsigned short> ||
    std::same_as<T, int> ||
    std::same_as<T, unsigned int> ||
    std::same_as<T, long> ||
    std::same_as<T, unsigned long> ||
    std::same_as<T, long long> ||
    std::same_as<T, unsigned long long> ||
    std::same_as<T, float> ||
    std::same_as<T, double> ||
    std::same_as<T, long double>;
template<from_chars_type T>
#endif
[[nodiscard]]
std::optional<T> from_str(std::string_view str){
    T result;
    [[maybe_unused]]
    auto [p, ec] {std::from_chars(
            str.data(), 
            str.data() + str.size(),
            result)
    };
    if(ec != std::errc{})
        return {};
    return result;
}

}
#endif // VI_H 
