#include "UDataBlockPool.h"
#include <type_traits>


namespace FrameProcessor {
    template <typename DataType>
    struct PoolAllocator {
        using value_type = DataType;
        PoolAllocator() = default;
        ~PoolAllocator() = default;

        template<typename U>
        constexpr PoolAllocator(const PoolAllocator<U>&) noexcept {}

        DataType* allocate(size_t n) {
            return reinterpret_cast<DataType>(UDataBlockPool::take(n));
        }

        void deallocate(DataType* p, std::size_t) noexcept {
            UDataBlockPool::release(reinterpret_cast<void*>(p));
        }

        template<typename U, typename... Args>
        void construct(U* p, Args&&... args) {
            new(p) U(std::forward<Args>(args)...);
        }

        template<typename U>
        void destroy(U* p) noexcept {
            p->~U();
        }
    };
    template <class T, class U>
    bool operator==(const PoolAllocator<T>&, const PoolAllocator<U>&) { return true; }
    template <class T, class U>
    bool operator!=(const PoolAllocator<T>&, const PoolAllocator<U>&) { return false; }
}
