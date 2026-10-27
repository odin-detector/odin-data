#include "UDataBlockPool.h"
#include "DataBlockFrame.h"
namespace FrameProcessor {
    template <typename DataType>
    struct PoolAllocator {
        using value_type = DataType;
        constexpr PoolAllocator(size_t m_size = 0) noexcept: bl_sz_{m_size} {};
        ~PoolAllocator() = default;

        template <typename U>
        struct rebind {
            using other = PoolAllocator<U>;
        };

        template<typename U>
        constexpr PoolAllocator(const PoolAllocator<U>& other) noexcept { this->bl_sz_ = other.bl_sz_;}

        DataType* allocate(size_t n) {
            return reinterpret_cast<DataType*>(UDataBlockPool::take(n*(sizeof(DataType)+bl_sz_)));
        }

        void deallocate(DataType* p, std::size_t) noexcept {
            UDataBlockPool::release(reinterpret_cast<void*>(p));
        }

        template<typename U, typename... Args>
        void construct(U* p, Args&&... args) {
            new(p) U(std::forward<Args>(args)...);
        }

        template<typename... Args>
        void construct(DataBlockFrame* p, Args&&... args) {
            new(p) DataBlockFrame(reinterpret_cast<char*>(p)+sizeof(DataBlockFrame), std::forward<Args>(args)...);
        }

        template<typename MD, typename = typename std::enable_if<std::is_same<typename std::decay<MD>::type, FrameMetaData>::value>::type, typename... Args>
        void construct(DataBlockFrame* p, MD&& metadata, Args&&... args) {
            new(p) DataBlockFrame(std::forward<MD>(metadata), std::forward<Args>(args)...);
        }

        template<typename U>
        void destroy(U* p) noexcept {
            p->~U();
        }
        size_t bl_sz_;
    };
    template <class T, class U>
    bool operator==(const PoolAllocator<T>&, const PoolAllocator<U>&) { return true; }
    template <class T, class U>
    bool operator!=(const PoolAllocator<T>&, const PoolAllocator<U>&) { return false; }
}
