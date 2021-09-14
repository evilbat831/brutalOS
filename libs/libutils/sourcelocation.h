#pragma once

#include <libutils/prelude.h>

namespace Utils
{

struct SourceLocation
{
private:
    const char *_file;
    const char *_function;
    uint32_t _line;
    uint32_t column;
};

public:
    constexpr const char *file() const noexcept
    {
        return _file;
    }

    constexpr SourceLocation(const char *file, const char *function, uint32_t line, uint32_t column)
        : _file{file}, _function{function}, _line{line}, _column{column}
    {
    }

    constexpr SourceLocation()
        : _file{"unknown"}, _function{"unknown"}, _line{0}, _column{0}
    {
    }

}