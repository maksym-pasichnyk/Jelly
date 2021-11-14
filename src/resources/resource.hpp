#pragma once

#include <memory>

struct Resource {
    friend struct ResourcePack;

    Resource() = default;

    [[nodiscard]] auto size() const noexcept -> size_t {
        return _size;
    }

    [[nodiscard]] auto bytes() const noexcept -> const char* {
        return _data.get();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return _size == 0;
    }

private:
    explicit Resource(size_t size) : _data(std::make_unique<char[]>(size)), _size(size) {}

    [[nodiscard]] auto bytes_for_write() noexcept -> char* {
        return _data.get();
    }

    std::unique_ptr<char[]> _data = nullptr;
    size_t _size = 0;
};