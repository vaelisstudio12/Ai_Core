// python.hpp
#pragma once

namespace python {

    static inline long sys_write(int fd, const void* buf, unsigned long len) {
        long ret;
        __asm__ volatile (
            "syscall"
            : "=a"(ret)
            : "a"(1), "D"(fd), "S"(buf), "d"(len)
            : "rcx", "r11", "memory"
        );
        return ret;
    }

    static inline long sys_read(int fd, void* buf, unsigned long len) {
        long ret;
        __asm__ volatile (
            "syscall"
            : "=a"(ret)
            : "a"(0), "D"(fd), "S"(buf), "d"(len)
            : "rcx", "r11", "memory"
        );
        return ret;
    }

    unsigned long length(const char* str) {
        if (!str) return 0; 
        const char* ptr = str;
        while (*ptr) { ptr++; }
        return (unsigned long)(ptr - str);
    }

    bool startswith(const char* str, const char* prefix) {
        if (!str || !prefix) return false; 
        while (*prefix) {
            if (*str != *prefix) return false;
            str++;
            prefix++;
        }
        return true;
    }

    bool endswith(const char* str, const char* suffix) {
        if (!str || !suffix) return false; 
        unsigned long str_len = length(str);
        unsigned long suffix_len = length(suffix);
        if (suffix_len > str_len) return false; 
        
        const char* str_start = str + (str_len - suffix_len);
        while (*suffix) {
            if (*str_start != *suffix) return false;
            str_start++;
            suffix++;
        }
        return true;
    }

    const char* lstrip(const char* str) {
        if (!str) return nullptr; 
        while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
            str++;
        }
        return str;
    }

    const char* rstrip(const char* str, unsigned long* out_len) {
        if (!str || !out_len) return nullptr;
        unsigned long len = length(str);
        while (len > 0) {
            char c = str[len - 1];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                len--;
            } else {
                break;
            }
        }
        *out_len = len; 
        return str;    
    }

    const char* strip(const char* str, unsigned long* out_len) {
        if (!str || !out_len) return nullptr; 
        const char* start = lstrip(str);
        return rstrip(start, out_len);
    }

    unsigned long lower(const char* src, char* dest, unsigned long max_len) {
        if (!src || !dest || max_len == 0) return 0;
        unsigned long i = 0;
        while (src[i] != '\0' && i < (max_len - 1)) {
            char c = src[i];
            dest[i] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
            i++;
        }
        dest[i] = '\0'; 
        return i; 
    }

    unsigned long upper(const char* src, char* dest, unsigned long max_len) {
        if (!src || !dest || max_len == 0) return 0;
        unsigned long i = 0;
        while (src[i] != '\0' && i < (max_len - 1)) {
            char c = src[i];
            dest[i] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
            i++;
        }
        dest[i] = '\0';
        return i;
    }

    unsigned long replace(const char* src, const char* old_str, const char* new_str, char* dest, unsigned long max_len) {
        if (!src || !old_str || !new_str || !dest || max_len == 0) return 0;
        unsigned long old_len = length(old_str);
        unsigned long new_len = length(new_str);
        if (old_len == 0) return 0;

        unsigned long src_i = 0;
        unsigned long dest_i = 0;

        while (src[src_i] != '\0' && dest_i < (max_len - 1)) {
            bool match = true;
            for (unsigned long j = 0; j < old_len; j++) {
                if (src[src_i + j] == '\0' || src[src_i + j] != old_str[j]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                for (unsigned long j = 0; j < new_len && dest_i < (max_len - 1); j++) {
                    dest[dest_i++] = new_str[j];
                }
                src_i += old_len;
            } else {
                dest[dest_i++] = src[src_i++];
            }
        }
        dest[dest_i] = '\0';
        return dest_i; 
    }

    long find(const char* str, const char* pattern, unsigned long start_pos = 0) {
        if (!str || !pattern) return -1;
        unsigned long str_len = length(str);
        unsigned long pat_len = length(pattern);

        if (pat_len == 0) return (start_pos <= str_len) ? (long)start_pos : -1;
        if (start_pos >= str_len || pat_len > str_len) return -1;

        unsigned long max_idx = str_len - pat_len;
        for (unsigned long i = start_pos; i <= max_idx; i++) {
            bool match = true;
            for (unsigned long j = 0; j < pat_len; j++) {
                if (str[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return (long)i;
        }
        return -1; 
    }

    long rfind(const char* str, const char* pattern) {
        if (!str || !pattern) return -1;
        unsigned long str_len = length(str);
        unsigned long pat_len = length(pattern);

        if (pat_len == 0) return (long)str_len;
        if (pat_len > str_len) return -1;

        unsigned long start = str_len - pat_len;
        for (long i = (long)start; i >= 0; i--) {
            bool match = true;
            for (unsigned long j = 0; j < pat_len; j++) {
                if (str[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return i;
        }
        return -1;
    }

    static inline void* sys_mmap(void* addr, unsigned long length, int prot, int flags, int fd, long offset) {
        void* ret;
        long r_flags = flags;
        long r_fd = fd;
        
        register long r10 __asm__("r10") = r_flags;
        register long r8  __asm__("r8")  = r_fd;
        register long r9  __asm__("r9")  = offset;

        __asm__ volatile (
            "syscall"
            : "=a"(ret)
            : "a"(9), "D"(addr), "S"(length), "d"(prot), "r"(r10), "r"(r8), "r"(r9)
            : "rcx", "r11", "memory"
        );
        return ret;
    }

    #define PROT_READ   0x1
    #define PROT_WRITE  0x2
    #define MAP_PRIVATE 0x02
    #define MAP_ANONYMOUS 0x20

    struct Arena {
        unsigned char* buffer;
        unsigned long capacity;
        unsigned long offset;

        static Arena create(unsigned long bytes) {
            Arena a;
            a.buffer = (unsigned char*)sys_mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            a.capacity = bytes;
            a.offset = 0;
            return a;
        }

        void* alloc(unsigned long bytes) {
            unsigned long aligned_bytes = (bytes + 7) & ~7;
            if (offset + aligned_bytes > capacity) return nullptr;
            void* ptr = &buffer[offset];
            offset += aligned_bytes;
            return ptr;
        }

        void reset() { offset = 0; }
    };

    struct Str {
        const char* ptr;
        unsigned long len;
    };

    template <typename T>
    struct List {
        T* items;
        unsigned long count;
        unsigned long capacity;
        Arena* arena;

        static List create(Arena* a, unsigned long initial_cap = 8) {
            List l;
            l.arena = a;
            l.count = 0;
            l.capacity = initial_cap;
            l.items = (T*)a->alloc(sizeof(T) * initial_cap);
            return l;
        }

        void append(T item) {
            if (count >= capacity) {
                unsigned long new_cap = capacity * 2;
                T* new_items = (T*)arena->alloc(sizeof(T) * new_cap);
                for (unsigned long i = 0; i < count; i++) {
                    new_items[i] = items[i];
                }
                items = new_items;
                capacity = new_cap;
            }
            items[count++] = item;
        }

        T pop() {
            if (count == 0) return T();
            return items[--count];
        }

        List slice(unsigned long start, unsigned long end) {
            List sub = List::create(arena, (end > start) ? (end - start) : 1);
            for (unsigned long i = start; i < end && i < count; i++) {
                sub.append(items[i]);
            }
            return sub;
        }
    };

    List<Str> split(Str src, char delim, Arena* arena) {
        List<Str> result = List<Str>::create(arena);
        unsigned long start = 0;

        for (unsigned long i = 0; i < src.len; i++) {
            if (src.ptr[i] == delim) {
                if (i > start) {
                    result.append(Str{src.ptr + start, i - start});
                }
                start = i + 1;
            }
        }
        if (start < src.len) {
            result.append(Str{src.ptr + start, src.len - start});
        }
        return result;
    }

    Str join(List<Str> list, Str delim, Arena* arena) {
        if (list.count == 0) return Str{"", 0};

        unsigned long total_len = 0;
        for (unsigned long i = 0; i < list.count; i++) {
            total_len += list.items[i].len;
            if (i < list.count - 1) total_len += delim.len;
        }

        char* buf = (char*)arena->alloc(total_len + 1);
        unsigned long curr = 0;

        for (unsigned long i = 0; i < list.count; i++) {
            for (unsigned long j = 0; j < list.items[i].len; j++) {
                buf[curr++] = list.items[i].ptr[j];
            }
            if (i < list.count - 1) {
                for (unsigned long j = 0; j < delim.len; j++) {
                    buf[curr++] = delim.ptr[j];
                }
            }
        }
        buf[curr] = '\0';
        return Str{buf, total_len};
    }

    Str format_int(long val, Arena* arena) {
        char buf[32];
        unsigned long i = 0;
        bool is_neg = false;

        if (val == 0) {
            char* res = (char*)arena->alloc(2);
            res[0] = '0'; res[1] = '\0';
            return Str{res, 1};
        }

        if (val < 0) {
            is_neg = true;
            val = -val;
        }

        while (val > 0) {
            buf[i++] = '0' + (val % 10);
            val /= 10;
        }
        if (is_neg) buf[i++] = '-';

        char* res = (char*)arena->alloc(i + 1);
        for (unsigned long j = 0; j < i; j++) {
            res[j] = buf[i - 1 - j];
        }
        res[i] = '\0';
        return Str{res, i};
    }

    Str format(Str fmt, Str arg1, Arena* arena) {
        if (!fmt.ptr || fmt.len == 0 || !arena) return Str{"", 0};

        long placeholder_pos = -1;
        if (fmt.len >= 2) {
            for (unsigned long i = 0; i <= fmt.len - 2; i++) {
                if (fmt.ptr[i] == '{' && fmt.ptr[i + 1] == '}') {
                    placeholder_pos = (long)i;
                    break;
                }
            }
        }

        if (placeholder_pos == -1) {
            char* buf = (char*)arena->alloc(fmt.len + 1);
            for (unsigned long i = 0; i < fmt.len; i++) buf[i] = fmt.ptr[i];
            buf[fmt.len] = '\0';
            return Str{buf, fmt.len};
        }

        unsigned long new_len = fmt.len - 2 + arg1.len;
        char* buf = (char*)arena->alloc(new_len + 1);
        unsigned long curr = 0;

        for (long i = 0; i < placeholder_pos; i++) {
            buf[curr++] = fmt.ptr[i];
        }
        for (unsigned long i = 0; i < arg1.len; i++) {
            buf[curr++] = arg1.ptr[i];
        }
        for (unsigned long i = placeholder_pos + 2; i < fmt.len; i++) {
            buf[curr++] = fmt.ptr[i];
        }

        buf[curr] = '\0';
        return Str{buf, new_len};
    }

    Str format(Str fmt, List<Str> args, Arena* arena) {
        if (!fmt.ptr || fmt.len == 0 || !arena) return Str{"", 0};
        Str current = fmt;
        for (unsigned long i = 0; i < args.count; i++) {
            current = format(current, args.items[i], arena);
        }
        return current;
    }

    Str format(Str fmt, long val, Arena* arena) {
        Str val_str = format_int(val, arena);
        return format(fmt, val_str, arena);
    }

} // namespace python