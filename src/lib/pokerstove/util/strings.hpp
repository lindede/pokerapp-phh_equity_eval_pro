#pragma once

#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <vector>

namespace pokerstove::util
{

inline void to_lower_inplace(std::string& s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

inline std::vector<std::string_view> split(std::string_view s, char delim)
{
    std::vector<std::string_view> parts;
    while (!s.empty())
    {
        const auto pos = s.find(delim);
        if (pos == std::string_view::npos)
        {
            parts.push_back(s);
            break;
        }
        parts.push_back(s.substr(0, pos));
        s.remove_prefix(pos + 1);
    }
    return parts;
}

inline bool contains(std::string_view s, std::string_view sub)
{
    return s.find(sub) != std::string_view::npos;
}

inline bool parse_double(std::string_view s, double& out)
{
    const std::string str(s);
    try
    {
        size_t idx = 0;
        out = std::stod(str, &idx);
        return idx == str.size();
    }
    catch (...)
    {
        return false;
    }
}

}  // namespace pokerstove::util
