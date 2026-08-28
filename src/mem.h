#pragma once

namespace mem
{
   class ArenaAllocator {
   public:
      explicit ArenaAllocator(std::size_t defaultBlockSize)
         : m_defaultBlockSize(defaultBlockSize)
      { m_head = new Block(defaultBlockSize); }

      ArenaAllocator(const ArenaAllocator&) noexcept = delete;
      ArenaAllocator(ArenaAllocator&& other) noexcept
         : m_head(other.m_head), m_defaultBlockSize(other.m_defaultBlockSize)
      { other.m_head = nullptr; }

      ArenaAllocator& operator=(const ArenaAllocator&) noexcept = delete;
      ArenaAllocator& operator=(ArenaAllocator&& other) noexcept; 

      ~ArenaAllocator() { freeChain(m_head); }

   public:
      /**
       * @brief frees every block except one, and rewinds that one to empty
       * @note collapses back down to m_defaultBlockSize worth of memory, same as a freshly-constructed Arena.
       */
      void reset() noexcept;

      /// @return total capacity across every block in chain
      [[nodiscard]] std::size_t capacity() const noexcept;

      /// @return total bytes currently in use across every block in chain
      [[nodiscard]] std::size_t used() const noexcept;

      /**
       * @return total remaining space across every block in the chain
       * @note NOT simply "one block's worth of free space" — a request could still fail to fit in any
       *       SINGLE block even if this total is large, since blocks aren't contiguous with each other.
       */
      [[nodiscard]] std::size_t available() const noexcept { return capacity() - used(); }

   public:
      /// @brief allocates and constructs T
      template<typename T, typename... Args>
      [[nodiscard]] T* create(Args&&... args) { // && implies: deduce whether each was an rvalue or lvalue and preserve that so std::forward works
         T* p_T = allocate<T>();
         return ::new (p_T) T(std::forward<Args>(args)...); // construct T and put it at p_T instead of wherever new usually puts it
      }

   private:
      /**
       * @brief only allocates memory, aligned as per T, NO construction involved
       * @returns address to allocated memory
       * @note grows into new block if current one is full
       */
      template<typename T>
      [[nodiscard]] T* allocate(std::size_t alignment = alignof(T)) {
         // so we can use the % operator
         const std::size_t currentAddress = reinterpret_cast<std::size_t>(m_head->buffer + m_head->offset);
         const std::size_t bytes = sizeof(T);

         // current % align = how far past the aligned boundary the current address sits
         // (alignment - that) % alignment to get the PADDING not raw value eg. if that = alignment then padding = 0
         const std::size_t offsetPadding = (alignment - (currentAddress % alignment)) % alignment;

         // does the current block have room? if not than grow
         if(m_head->offset + offsetPadding + bytes > m_head->capacity) {
            std::size_t newBlockSize = std::max(m_defaultBlockSize, bytes + alignment); // guard against request bigger than default
            Block* newBlock = new Block(newBlockSize);
            newBlock->next = m_head;
            m_head = newBlock;

            return allocate<T>(alignment); // retry with fresh block
         }

         m_head->offset += offsetPadding;
         void* ptr = m_head->buffer + m_head->offset;
         m_head->offset += bytes;

         return static_cast<T*>(ptr);
      }

   private:
      struct Block {
         std::byte* buffer = nullptr;
         std::size_t capacity = 0;
         std::size_t offset = 0;
         Block* next = nullptr; // next block in the chain (older block)

         /** explanation of ::operator new instead of new
          * c++ modern equivalent of malloc that throws
          * allocate without calling any constructors
          * buffer now points to the beginning of our memory block
          */
         explicit Block(std::size_t capacity)
            : buffer(static_cast<std::byte*>(::operator new(capacity))), capacity(capacity) {}

         ~Block() { ::operator delete(buffer); } // delete without calling any destructors
      };

      Block* m_head = nullptr; /// current (most recent) block. All allocation happens here
      std::size_t m_defaultBlockSize; /// size for new blocks when we grow

   private:
      void freeChain(Block* b) noexcept; 
   };
}
