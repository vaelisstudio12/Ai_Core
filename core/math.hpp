// math.hpp
#pragma once
#include "python.hpp"

namespace math {

    inline long abs(long v) { 
        return v < 0 ? -v : v; 
    }

    inline long min(long a, long b) { 
        return a < b ? a : b; 
    }

    inline long max(long a, long b) { 
        return a > b ? a : b; 
    }

    // Elemanların toplamı
    inline long sum(python::List<long> list) {
        long total = 0;
        for (unsigned long i = 0; i < list.count; i++) {
            total += list.items[i];
        }
        return total;
    }

    // Elemanların çarpımı
    inline long product(python::List<long> list) {
        if (list.count == 0) return 0;
        long prod = 1;
        for (unsigned long i = 0; i < list.count; i++) {
            prod *= list.items[i];
        }
        return prod;
    }

    // Aritmetik ortalama
    inline long average(python::List<long> list) {
        if (list.count == 0) return 0;
        return sum(list) / (long)list.count;
    }

    // Tam sayı üs alma (Power)
    inline long pow(long base, long exp) {
        if (exp < 0) return 0;
        long result = 1;
        while (exp > 0) {
            if (exp & 1) result *= base;
            base *= base;
            exp >>= 1;
        }
        return result;
    }

    // Bitsel İşlemler (Bitwise Operators - Lisp AST için)
    inline long bit_and(long a, long b) { return a & b; }
    inline long bit_or(long a, long b)  { return a | b; }
    inline long bit_xor(long a, long b) { return a ^ b; }
    inline long bit_shl(long a, int shift) { return a << shift; }
    inline long bit_shr(long a, int shift) { return a >> shift; }

    // Güvenli Range Üretecimiz (step == 0 korumalı)
    inline python::List<long> range(long start, long end, python::Arena* arena, long step = 1) {
        python::List<long> res = python::List<long>::create(arena);
        if (step == 0) return res; // Sonsuz döngü koruması

        if (step > 0) {
            for (long i = start; i < end; i += step) {
                res.append(i);
            }
        } else if (step < 0) {
            for (long i = start; i > end; i += step) {
                res.append(i);
            }
        }
        return res;
    }

} // namespace math