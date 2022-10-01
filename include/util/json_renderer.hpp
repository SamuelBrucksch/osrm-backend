// based on
// https://svn.apache.org/repos/asf/mesos/tags/release-0.9.0-incubating-RC0/src/common/json.hpp

#ifndef JSON_RENDERER_HPP
#define JSON_RENDERER_HPP

#include "util/string_util.hpp"

#include "osrm/json_container.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

#include <fmt/compile.h>

namespace osrm
{
namespace util
{
namespace json
{

template <typename Out> struct Renderer
{
    explicit Renderer(Out &_out) : out(_out) {}

    void operator()(const String &string)
    {
        write('"');
        // here we assume that vast majority of strings don't need to be escaped,
        // so we check it first and escape only if needed
        auto size = SizeOfEscapedJSONString(string.value);
        if (size == string.value.size())
        {
            write(string.value);
        }
        else
        {
            std::string escaped;
            escaped.reserve(size);
            EscapeJSONString(string.value, escaped);

            write(escaped);
        }
        write('"');
    }

    void operator()(const Number &number)
    {
        // `fmt::memory_buffer` stores first 500 bytes in the object itself(i.e. on stack in this
        // case) and then grows using heap if needed
        fmt::memory_buffer buffer;
        fmt::format_to(std::back_inserter(buffer), FMT_COMPILE("{}"), number.value);

        // Truncate to 10 decimal places
        size_t decimalpos = std::find(buffer.begin(), buffer.end(), '.') - buffer.begin();
        if (buffer.size() > (decimalpos + 10))
        {
            buffer.resize(decimalpos + 10);
        }

        write(buffer.data(), buffer.size());
    }

    void operator()(const Object &object)
    {
        write('{');
        for (auto it = object.values.begin(), end = object.values.end(); it != end;)
        {
            write('\"');
            write(it->first);
            write("\":");
            mapbox::util::apply_visitor(Renderer(out), it->second);
            if (++it != end)
            {
                write(',');
            }
        }
        write('}');
    }

    void operator()(const Array &array)
    {
        write('[');
        for (auto it = array.values.cbegin(), end = array.values.cend(); it != end;)
        {
            mapbox::util::apply_visitor(Renderer(out), *it);
            if (++it != end)
            {
                write(',');
            }
        }
        write(']');
    }

    void operator()(const True &) { write<>("true"); }

    void operator()(const False &) { write<>("false"); }

    void operator()(const Null &) { write<>("null"); }

  private:
    void write(const std::string &str);
    void write(const char *str, size_t size);
    void write(char ch);

    template <size_t StrLength> void write(const char (&str)[StrLength])
    {
        write(str, StrLength - 1);
    }

  private:
    Out &out;
};

template <> void Renderer<std::vector<char>>::write(const std::string &str)
{
    out.insert(out.end(), str.begin(), str.end());
}

template <> void Renderer<std::vector<char>>::write(const char *str, size_t size)
{
    out.insert(out.end(), str, str + size);
}

template <> void Renderer<std::vector<char>>::write(char ch) { out.push_back(ch); }

template <> void Renderer<std::ostream>::write(const std::string &str) { out << str; }

template <> void Renderer<std::ostream>::write(const char *str, size_t size)
{
    out.write(str, size);
}

template <> void Renderer<std::ostream>::write(char ch) { out << ch; }

template <> void Renderer<std::string>::write(const std::string &str) { out += str; }

template <> void Renderer<std::string>::write(const char *str, size_t size)
{
    out.append(str, size);
}

template <> void Renderer<std::string>::write(char ch) { out += ch; }

inline void render(std::ostream &out, const Object &object)
{
    Renderer renderer(out);
    renderer(object);
}

inline void render(std::string &out, const Object &object)
{
    Renderer renderer(out);
    renderer(object);
}

inline void render(std::vector<char> &out, const Object &object)
{
    Renderer renderer(out);
    renderer(object);
}

} // namespace json
} // namespace util
} // namespace osrm

#endif // JSON_RENDERER_HPP
