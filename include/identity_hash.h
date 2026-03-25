#pragma once

#include <cstdint>

struct IdentityHash {
    size_t operator()(uint32_t k) const {
        return k;
    }
};