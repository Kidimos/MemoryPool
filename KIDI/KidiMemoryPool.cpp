// #include "KidiMemoryPool.h"

// /**
//  * PUBLIC
//  */

// template <typename T, size_t BlockSize>
// inline KidiMemoryPool<T, BlockSize>::KidiMemoryPool() noexcept 
//     : current_block_(nullptr), current_slot_(nullptr), last_slot_(nullptr), free_slot_(nullptr){}

// template <typename T, size_t BlockSize>
// KidiMemoryPool<T, BlockSize>::KidiMemoryPool(const KidiMemoryPool &memoryPool) noexcept {
//     KidiMemoryPool();
// }

// template <typename T, size_t BlockSize>
// KidiMemoryPool<T, BlockSize>::KidiMemoryPool(KidiMemoryPool &&memoryPool) noexcept 
//     : current_block_(memoryPool.current_block_), current_slot_(memoryPool.current_slot_),
//      last_slot_(memoryPool.last_slot_), free_slot_(memoryPool.free_slot_){
//         memoryPool.current_block_ = nullptr;
//         // memoryPool.current_slot_ = nullptr;
//         // memoryPool.last_slot_ = nullptr;
//         // memoryPool.free_slot_ = nullptr;
// }

// template <typename T, size_t BlockSize>
// template <class U>
// KidiMemoryPool<T, BlockSize>::KidiMemoryPool(const KidiMemoryPool<U>& memoryPool) noexcept{
//     KidiMemoryPool();
// }

// template <typename T, size_t BlockSize>
// KidiMemoryPool<T, BlockSize>::~KidiMemoryPool() noexcept {
//     slot_pointer_ curr = current_block_;
//     while (curr != nullptr){
//         slot_pointer_ prev = curr->next;
//         operator delete(reinterpret_cast<void*>(curr));
//         curr = prev;
//     }
// }

// template <typename T, size_t BlockSize>
// KidiMemoryPool<T, BlockSize>&
// KidiMemoryPool<T, BlockSize>::operator=(KidiMemoryPool&& memoryPool) noexcept {
//     if (this != &memoryPool){
//         std::swap(current_block_, memoryPool.current_block_);
//         current_slot_ = memoryPool.current_slot_;
//         last_slot_ = memoryPool.last_slot_;
//         free_slot_ = memoryPool.free_slot_;
//     }
// }

// template <typename T, size_t BlockSize>
// inline typename KidiMemoryPool<T, BlockSize>::pointer
// KidiMemoryPool<T, BlockSize>::address(reference x) const noexcept{
//     return &x;
// }

// template <typename T, size_t BlockSize>
// inline typename KidiMemoryPool<T, BlockSize>::pointer
// KidiMemoryPool<T, BlockSize>::address(const_reference x) const noexcept{
//     return &x;
// }

// template <typename T, size_t BlockSize>
// inline typename KidiMemoryPool<T, BlockSize>::pointer
// KidiMemoryPool<T, BlockSize>::allocate(size_type n = 1, const_pointer hint = 0){
//     if (free_slot_ != nullptr){
//         pointer result = reinterpret_cast<pointer>(free_slot_);
//         free_slot_ = free_slot_->next;
//         return result;
//     }else {
//         if(current_slot_ >= last_slot_){
//             allocateBlock();
//         }
//         return reinterpret_cast<pointer>(current_slot_++);
//     }
// }

// template <typename T, size_t BlockSize>
// void KidiMemoryPool<T, BlockSize>::deallocate(pointer p, size_type n = 1){
//     if(p != nullptr){
//         reinterpret_cast<slot_pointer_>(p)->next = free_slot_;
//         free_slot_ = reinterpret_cast<slot_pointer_>(p);
//     }
// }

// template <typename T, size_t BlockSize>
// inline typename KidiMemoryPool<T, BlockSize>::size_type
// KidiMemoryPool<T, BlockSize>::max_size() const noexcept{
//     size_type maxBlocks = -1 / BlockSize;
//     return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_pointer_) * maxBlocks;
// }

// template <typename T, size_t BlockSize>
// template <class U, class... Args>
// inline void KidiMemoryPool<T, BlockSize>::construct(U* p, Args&&... args){
//     new (p) U (std::forward<Args>(args)...);
// }

// template <typename T, size_t BlockSize>
// template <class U>
// inline void KidiMemoryPool<T, BlockSize>::destroy(U* p){
//     p->~U();
// }

// template <typename T, size_t BlockSize>
// template <class... Args>
// inline typename KidiMemoryPool<T, BlockSize>::pointer
// KidiMemoryPool<T, BlockSize>::newElement(Args&&... args){
//     pointer result = allocate();
//     construct<value_type>(result, std::forward<Args>(args)...);
//     return result;
// }

// template <typename T, size_t BlockSize>
// void KidiMemoryPool<T, BlockSize>::deleteElement(pointer p){
//     if(p != nullptr){
//         p->~value_type();
//         deallocate(p);
//     }
// }

// /**
//  * PRIVATE
//  */

// template <typename T, size_t BlockSize>
// inline typename KidiMemoryPool<T, BlockSize>::size_type
// KidiMemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align) const noexcept{
//     uintptr_t result = reinterpret_cast<uintptr_t>(p);
//     return (align - result) % align;
// }

// template <typename T, size_t BlockSize>
// void KidiMemoryPool<T, BlockSize>::allocateBlock(){
//     // Allocate space for the new block and store a pointer to the previous one
//     data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize));
//     reinterpret_cast<slot_pointer_>(newBlock)->next = current_block_;
//     current_block_ = reinterpret_cast<slot_pointer_>(newBlock);
//     // Pad block body to staisfy the alignment requirements for elements
//     data_pointer_ body = newBlock + sizeof(slot_pointer_);
//     size_type bodyPadding = padPointer(body, sizeof(slot_type_));
//     current_slot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
//     last_slot_ = reinterpret_cast<slot_pointer_>(newBlock + BlockSize - sizeof(slot_type_) + 1);
// }