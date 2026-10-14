
#pragma once

/**
 * 使用 mmap 进行栈分配器
 * 懒加载，只在需要时分配内存   
 */

#include <cstddef>
#include <cstdint>

namespace cpp_toolkit
{

class StackAllocator
{
public:
    static void* Alloc(size_t size);
    static void Free(void* ptr);
};

} // namespace cpp_toolkit