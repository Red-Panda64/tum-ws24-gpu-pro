#pragma once

template<typename T>
T ceilDiv(T x, T y) {
    return (x + y - 1) / y;
}
