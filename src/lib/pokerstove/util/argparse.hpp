#pragma once

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pokerstove
{

class ArgParser
{
public:
    void set_description(std::string desc) { description_ = std::move(desc); }

    void add_option(const std::string& long_name,
                    char short_name = '\0',
                    bool has_value = false,
                    const std::string& default_value = {},
                    bool is_flag = false)
    {
        Option opt;
        opt.long_name = long_name;
        opt.short_name = short_name;
        opt.has_value = has_value;
        opt.is_flag = is_flag;
        opt.default_value = default_value;
        options_.push_back(opt);
        if (!default_value.empty() || has_value)
            values_[long_name] = default_value;
    }

    void add_positional(const std::string& name)
    {
        Option opt;
        opt.long_name = name;
        opt.is_positional = true;
        opt.has_value = true;
        options_.push_back(opt);
    }

    bool parse(int argc, char** argv)
    {
        positional_values_.clear();
        flags_.clear();

        for (const auto& opt : options_)
        {
            if (opt.is_flag)
                flags_[opt.long_name] = false;
            else if (opt.has_value && !opt.is_positional && !opt.default_value.empty())
                values_[opt.long_name] = opt.default_value;
        }

        std::vector<std::string> positional;
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--")
            {
                for (++i; i < argc; ++i)
                    positional.push_back(argv[i]);
                break;
            }

            if (arg.size() >= 2 && arg[0] == '-' && arg[1] != '-')
            {
                if (arg.size() > 2)
                {
                    for (size_t j = 1; j < arg.size(); ++j)
                    {
                        if (!apply_short(arg[j], j + 1 < arg.size() ? arg.substr(j + 1) : std::string{},
                                         i, argc, argv))
                            return false;
                        break;
                    }
                    continue;
                }

                if (!apply_short(arg[1], i + 1 < argc ? argv[i + 1] : std::string{}, i, argc, argv))
                    return false;
                if (needs_value_after_short(arg[1]))
                    ++i;
                continue;
            }

            if (arg.size() >= 3 && arg.substr(0, 2) == "--")
            {
                std::string key = arg.substr(2);
                std::string val;
                const auto eq = key.find('=');
                if (eq != std::string::npos)
                {
                    val = key.substr(eq + 1);
                    key = key.substr(0, eq);
                }
                if (!apply_long(key, val, i, argc, argv))
                    return false;
                if (val.empty() && needs_value_after_long(key))
                    ++i;
                continue;
            }

            positional.push_back(arg);
        }

        for (const auto& opt : options_)
        {
            if (opt.is_positional)
                positional_values_ = positional;
        }
        return true;
    }

    bool flag(const std::string& name) const
    {
        const auto it = flags_.find(name);
        return it != flags_.end() && it->second;
    }

    bool has(const std::string& name) const
    {
        if (flag(name))
            return true;
        const auto it = values_.find(name);
        return it != values_.end() && !it->second.empty();
    }

    std::string get(const std::string& name) const
    {
        const auto it = values_.find(name);
        if (it != values_.end())
            return it->second;
        for (const auto& opt : options_)
            if (opt.long_name == name)
                return opt.default_value;
        return {};
    }

    const std::vector<std::string>& positional() const { return positional_values_; }

    void print_help(std::ostream& out) const
    {
        out << description_ << "\n\nOptions:\n";
        for (const auto& opt : options_)
        {
            if (opt.is_positional)
                continue;
            out << "  ";
            if (opt.short_name)
                out << "-" << opt.short_name << ", ";
            out << "--" << opt.long_name;
            if (opt.has_value && !opt.is_flag)
                out << " arg";
            if (!opt.default_value.empty())
                out << " (=" << opt.default_value << ")";
            out << "\n";
        }
    }

private:
    struct Option
    {
        std::string long_name;
        char short_name = '\0';
        bool has_value = false;
        bool is_flag = false;
        bool is_positional = false;
        std::string default_value;
    };

    const Option* find_long(const std::string& name) const
    {
        for (const auto& opt : options_)
            if (!opt.is_positional && opt.long_name == name)
                return &opt;
        return nullptr;
    }

    const Option* find_short(char c) const
    {
        for (const auto& opt : options_)
            if (!opt.is_positional && opt.short_name == c)
                return &opt;
        return nullptr;
    }

    bool needs_value_after_short(char c) const
    {
        const Option* opt = find_short(c);
        return opt && opt->has_value && !opt->is_flag;
    }

    bool needs_value_after_long(const std::string& name) const
    {
        const Option* opt = find_long(name);
        return opt && opt->has_value && !opt->is_flag;
    }

    bool apply_short(char c, const std::string& attached, int& i, int argc, char** argv)
    {
        const Option* opt = find_short(c);
        if (!opt)
            return false;
        if (opt->is_flag)
        {
            flags_[opt->long_name] = true;
            return true;
        }
        if (!opt->has_value)
            return false;
        std::string val = attached;
        if (val.empty())
        {
            if (i + 1 >= argc)
                return false;
            val = argv[++i];
        }
        values_[opt->long_name] = val;
        return true;
    }

    bool apply_long(const std::string& name, std::string val, int& i, int argc, char** argv)
    {
        const Option* opt = find_long(name);
        if (!opt)
            return false;
        if (opt->is_flag)
        {
            flags_[opt->long_name] = true;
            return true;
        }
        if (!opt->has_value)
            return false;
        if (val.empty())
        {
            if (i + 1 >= argc)
                return false;
            val = argv[++i];
        }
        values_[opt->long_name] = val;
        return true;
    }

    std::string description_;
    std::vector<Option> options_;
    std::map<std::string, std::string> values_;
    std::map<std::string, bool> flags_;
    std::vector<std::string> positional_values_;
};

}  // namespace pokerstove
