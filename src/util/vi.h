#ifndef VI_H
#define VI_H

#include <vector>
#include <charconv>
#include <string_view>
#include <cstddef>
#include <system_error>
#include <optional>

#include <concepts>

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

constexpr void for_each_split_str(
        std::string_view str,
        const std::string_view delim,
        const std::invocable<std::string_view> auto& func)
{
    if(delim == ""){
        func(std::string_view{str.begin(), str.end()});
        return;
    }
    auto i{str.begin()};
    auto j{find_str_in_str(str, delim)};
    while(j != str.end()){
        func(std::string_view{i, j});
        i = j + delim.size();
        str = {i, str.end()};
        j = {find_str_in_str(str, delim)};
    }
    func(std::string_view{i, str.end()});
}

constexpr void for_each_split_str_terminate(
        std::string str,
        const std::string_view delim_chars,
        const std::invocable<std::string_view> auto& func)
{
    if(delim_chars == ""){
        func(std::string_view{str.begin(), str.end()});
        return;
    }
    auto i{str.begin()};
    auto j{str.begin()};
    while(i != str.end()){
        bool split_str_now = false;
        for (char delim_char : delim_chars) {
            if (delim_char == *i) {
                split_str_now = true;
                break;
            }
        }
        if (split_str_now) {
            *i = '\0';
            func(std::string_view{j, i});
            j = i+1;
        }
        i++;
    }
    func(std::string_view{j, str.end()});
}

// Same as for_each_split_str but delim_chars is a list of char delimeters
constexpr void for_each_split_str_delim_chars(
        std::string_view str,
        const std::string_view delim_chars,
        const std::invocable<std::string_view> auto& func)
{
    if(delim_chars == ""){
        func(std::string_view{str.begin(), str.end()});
        return;
    }
    auto i{str.begin()};
    auto j{str.begin()};
    while(i != str.end()){
        bool split_str_now = false;
        for (char delim_char : delim_chars) {
            if (delim_char == *i) {
                split_str_now = true;
                break;
            }
        }
        if (split_str_now) {
            func(std::string_view{j, i});
            j = i+1;
        }
        i++;
    }
    func(std::string_view{j, str.end()});
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

// Same as split_str but delim_chars is a list of char delimeters
[[nodiscard]]
inline auto split_str_delim_chars(
        const std::string_view str,
        const std::string_view delim_chars)
{
    std::vector<std::string_view> v;
    for_each_split_str_delim_chars(str, delim_chars, [&v](auto s){
        v.push_back(s);
    });
    return v;
}

// Same as split_str_delim_chars but null terminates substrings so they are valid c strings. str argument is modified.
[[nodiscard]]
inline auto split_str_terminate(
        std::string str,
        const std::string_view delim)
{
    std::vector<std::string_view> v;
    for_each_split_str_terminate(str, delim, [&v](auto s){
        v.push_back(s);
    });
    return v;
}

// only fundamental types compatible with std::from_chars
template<typename T>
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
[[nodiscard]]
inline std::optional<T> from_str(std::string_view str){
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

struct unexpected_type { // because posix reserves _t suffix for some reason
    explicit constexpr unexpected_type(int) {}
};

inline constexpr unexpected_type unexpected{0};

template<typename T, typename U>
concept expected_value_or_type = 
    std::copy_constructible<T> &&
    std::move_constructible<T> &&
    std::convertible_to<T&&, U>;

template<typename T>
concept valid_optional = requires (T x) {
    std::optional{x};
};

template<valid_optional T, std::default_initializable E>
class expected {
    
    std::optional<T> opt{};
    E err{};

    public:

    constexpr expected() = default;
    template<typename U = T>
    constexpr expected(U&& arg) : opt{arg} {}
    constexpr expected([[maybe_unused]] unexpected_type t) : opt{} {}
    constexpr expected([[maybe_unused]] unexpected_type t, E e) : opt{}, err{e} {}
    template<typename U = T>
    constexpr expected(const std::optional<U>& other) : opt{other} {}
    template<typename U = T>
    constexpr expected(std::optional<U>&& other) noexcept : opt{other} {}

    constexpr explicit operator bool() const noexcept { return opt.has_value(); }
    constexpr bool has_value() const noexcept { return opt.has_value(); }

    constexpr operator std::optional<T>() const noexcept { return opt; }

    constexpr const T* operator->() const noexcept { return opt; }
    constexpr T* operator->() noexcept { return &*opt; }

    constexpr const T& operator*() const& noexcept { return *opt; }
    constexpr T& operator*() & noexcept { return *opt; }
    
    constexpr const T&& operator*() const&& noexcept { return *opt; }
    constexpr T&& operator*() && noexcept { return *opt; }

    constexpr const T& value() const& { return opt.value(); }
    constexpr T& value() & { return opt.value(); }

    constexpr const T&& value() const&& { return opt.value(); }
    constexpr T&& value() && { return opt.value(); }

    constexpr T value_or(expected_value_or_type<T> auto&& default_value) const& { return opt.value_or(default_value); }
    constexpr T value_or(expected_value_or_type<T> auto&& default_value) && { return opt.value_or(default_value); }

    constexpr const E& error() const& { return err; }
    constexpr E& error() & { return err; }

};

}
#endif // VI_H 
