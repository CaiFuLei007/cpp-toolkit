
#include "stack_allocator.h"
#include <sys/mman.h>



namespace cpp_toolkit
{


void* StackAllocator::Alloc(size_t size)
{
    void* stack = mmap(nullptr, size +sizeof(size_t), PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(stack == MAP_FAILED)
    {
        return nullptr;
    }
    *(size_t*)stack = size;

    return (void*)((char*)stack + sizeof(size_t));
}

void StackAllocator::Free(void* ptr)
{
    size_t size = *(size_t*)((char*)ptr - sizeof(size_t));
    void* base = (char*)ptr - sizeof(size_t);
    munmap(base, size + sizeof(size_t));
}


} // namespace cpp_toolkit