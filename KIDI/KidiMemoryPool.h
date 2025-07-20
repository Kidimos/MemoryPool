#ifndef KIDI_MEMORY_POOL_H
#define KIDI_MEMORY_POOL_H

#include <climits>
#include <cstddef>
#include <utility>
#include <mutex>
#include <iostream>

#define KIDI_POOL_THREAD_SAFE 0
#define DEBUG 0

template <typename T, size_t BlockSize = 4096>
class KidiMemoryPool{
public:
    typedef T                   value_type;
    typedef T*                  pointer;
    typedef T&                  reference;
    typedef const T*            const_pointer;
    typedef const T&            const_reference;
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;

    // 是否传播分配器
    typedef std::false_type     propagate_on_container_copy_assignment;
    typedef std::true_type      propagate_on_container_move_assignment;
    typedef std::true_type      propagate_on_container_swap;

    static constexpr size_t MIN_BLOCK_SIZE = 256;
    static constexpr size_t MAX_BLOCK_SIZE = 1024 * 1024;

    template <typename U> struct rebind{
        typedef KidiMemoryPool<U> other;
    };
    // using rebind_alloc = KidiMemoryPool<U>;

    KidiMemoryPool() noexcept;
    KidiMemoryPool(const KidiMemoryPool& memoryPool) noexcept;
    KidiMemoryPool(KidiMemoryPool&& memoryPool) noexcept;
    template <class U>
    KidiMemoryPool(const KidiMemoryPool<U>& memoryPool) noexcept;

    ~KidiMemoryPool() noexcept;

    KidiMemoryPool& operator=(const KidiMemoryPool& memoryPool) = delete;
    KidiMemoryPool& operator=(KidiMemoryPool&& memoryPool) noexcept;

    pointer address(reference x) const noexcept;
    const_pointer address(const_reference x) const noexcept;

    // Can Only allocate one object at a time, n and hint are ignored
    pointer allocate(size_type n = 1, const_pointer hint = 0);
    void deallocate(pointer p, size_type n = 1);

    size_type max_size() const noexcept;

    template <class U, class... Args> void construct(U* p, Args&&... args);
    template <class U> void destroy(U* p);

    template <class... Args> pointer newElement(Args&&... args);
    void deleteElement(pointer p);

private:
    union Slot_{
        value_type element;
        Slot_* next;
    };

    typedef char*   data_pointer_;
    typedef Slot_   slot_type_;
    typedef Slot_*  slot_pointer_;

    slot_pointer_   current_block_;
    slot_pointer_   current_slot_;
    slot_pointer_   last_slot_;
    slot_pointer_   free_slot_;



#if KIDI_POOL_THREAD_SAFE
    std::mutex mtx_block_;
    std::mutex mtx_free_;
    std::mutex mtx_curr_;
#endif

    size_t slot_count_per_block_;

    size_type padPointer(data_pointer_ p, size_type align) const noexcept;
    void allocateBlock();

    static_assert(BlockSize >= 2 * sizeof(slot_type_) , "BlockSize too small.");
};

template <typename T, size_t BlockSize>
inline KidiMemoryPool<T, BlockSize>::KidiMemoryPool() noexcept 
    : current_block_(nullptr), current_slot_(nullptr), last_slot_(nullptr), free_slot_(nullptr){
        // calculateSlotCount();
    }

template <typename T, size_t BlockSize>
KidiMemoryPool<T, BlockSize>::KidiMemoryPool(const KidiMemoryPool &memoryPool) noexcept {
    KidiMemoryPool();
}

template <typename T, size_t BlockSize>
KidiMemoryPool<T, BlockSize>::KidiMemoryPool(KidiMemoryPool &&memoryPool) noexcept 
    : current_block_(memoryPool.current_block_), current_slot_(memoryPool.current_slot_),
     last_slot_(memoryPool.last_slot_), free_slot_(memoryPool.free_slot_){
        memoryPool.current_block_ = nullptr;
        memoryPool.current_slot_ = nullptr;
        memoryPool.last_slot_ = nullptr;
        memoryPool.free_slot_ = nullptr;
}

template <typename T, size_t BlockSize>
template <class U>
KidiMemoryPool<T, BlockSize>::KidiMemoryPool(const KidiMemoryPool<U>& memoryPool) noexcept{
    KidiMemoryPool();
}

template <typename T, size_t BlockSize>
KidiMemoryPool<T, BlockSize>::~KidiMemoryPool() noexcept {
// #if KIDI_POOL_THREAD_SAFE
//     std::lock_guard<std::mutex> lock(mtx_);
// #endif

    slot_pointer_ curr = current_block_;
    while (curr != nullptr){
        slot_pointer_ prev = curr->next;
        operator delete(reinterpret_cast<void*>(curr));
        curr = prev;
    }
}

template <typename T, size_t BlockSize>
KidiMemoryPool<T, BlockSize>&
KidiMemoryPool<T, BlockSize>::operator=(KidiMemoryPool&& memoryPool) noexcept {
    if (this != &memoryPool){
        std::swap(current_block_, memoryPool.current_block_);
        current_slot_ = memoryPool.current_slot_;
        last_slot_ = memoryPool.last_slot_;
        free_slot_ = memoryPool.free_slot_;
    }
}

template <typename T, size_t BlockSize>
inline typename KidiMemoryPool<T, BlockSize>::pointer
KidiMemoryPool<T, BlockSize>::address(reference x) const noexcept{
    return &x;
}

template <typename T, size_t BlockSize>
inline typename KidiMemoryPool<T, BlockSize>::const_pointer
KidiMemoryPool<T, BlockSize>::address(const_reference x) const noexcept{
    return &x;
}

template <typename T, size_t BlockSize>
inline typename KidiMemoryPool<T, BlockSize>::pointer
KidiMemoryPool<T, BlockSize>::allocate(size_type n, const_pointer hint){
    if (free_slot_ != nullptr){
    #if KIDI_POOL_THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_free_);
    #endif 
        pointer result = reinterpret_cast<pointer>(free_slot_);
        free_slot_ = free_slot_->next;
        return result;
    }else {
    #if KIDI_POOL_THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_curr_);
    #endif 
        if(current_slot_ >= last_slot_){
            allocateBlock();
        }
        return reinterpret_cast<pointer>(current_slot_++);
    }
}

template <typename T, size_t BlockSize>
void KidiMemoryPool<T, BlockSize>::deallocate(pointer p, size_type n){
    if(p != nullptr){
    #if KIDI_POOL_THREAD_SAFE
        std::lock_guard<std::mutex> lock(mtx_free_);
    #endif 
        reinterpret_cast<slot_pointer_>(p)->next = free_slot_;
        free_slot_ = reinterpret_cast<slot_pointer_>(p);
    }
}

template <typename T, size_t BlockSize>
inline typename KidiMemoryPool<T, BlockSize>::size_type
KidiMemoryPool<T, BlockSize>::max_size() const noexcept{
    size_type maxBlocks = -1 / BlockSize;
    return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_pointer_) * maxBlocks;
}

template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void KidiMemoryPool<T, BlockSize>::construct(U* p, Args&&... args){
    new (p) U (std::forward<Args>(args)...);
}

template <typename T, size_t BlockSize>
template <class U>
inline void KidiMemoryPool<T, BlockSize>::destroy(U* p){
    p->~U();
}

template <typename T, size_t BlockSize>
template <class... Args>
inline typename KidiMemoryPool<T, BlockSize>::pointer
KidiMemoryPool<T, BlockSize>::newElement(Args&&... args){
    pointer result = allocate();
    construct<value_type>(result, std::forward<Args>(args)...);
    return result;
}

template <typename T, size_t BlockSize>
void KidiMemoryPool<T, BlockSize>::deleteElement(pointer p){
    if(p != nullptr){
        p->~value_type();
        deallocate(p);
    }
}

/**
 * PRIVATE
 */

template <typename T, size_t BlockSize>
inline typename KidiMemoryPool<T, BlockSize>::size_type
KidiMemoryPool<T, BlockSize>::padPointer(data_pointer_ p,
                                         size_type align) const noexcept {
  uintptr_t result = reinterpret_cast<uintptr_t>(p);
  return (align - result) % align;
}

template <typename T, size_t BlockSize>
void KidiMemoryPool<T, BlockSize>::allocateBlock(){
#if KIDI_POOL_THREAD_SAFE
    std::lock_guard<std::mutex> lock(mtx_block_);
#endif
    // Allocate space for the new block and store a pointer to the previous one
    data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize));
    reinterpret_cast<slot_pointer_>(newBlock)->next = current_block_;
    current_block_ = reinterpret_cast<slot_pointer_>(newBlock);
    
    // Pad block body to staisfy the alignment requirements for elements
    data_pointer_ body = reinterpret_cast<data_pointer_>(current_block_) + sizeof(slot_pointer_);
    size_type bodyPadding = padPointer(body, sizeof(slot_type_));
    current_slot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    last_slot_ = reinterpret_cast<slot_pointer_>(reinterpret_cast<data_pointer_>(current_block_) + BlockSize - sizeof(slot_type_));
}

#endif //KIDI_MEMORY_POOL_H