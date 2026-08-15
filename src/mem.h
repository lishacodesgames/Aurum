#pragma once
#include "Errors.h"

namespace mem
{
   using std::size_t;

   class ArenaAllocator {
   public:
      explicit ArenaAllocator(size_t capacity) : m_capacity(capacity) {
         // c++ modern equivalent of malloc that throws accordingly
         // allocate without calling any constructors
         // m_buffer now points to the beginning of our memory block
         m_buffer = static_cast<std::byte*>(::operator new(m_capacity));
      }

      ArenaAllocator(const ArenaAllocator&) = delete;
      ArenaAllocator(ArenaAllocator&& other) noexcept
         : m_buffer(other.m_buffer), m_capacity(other.m_capacity), m_offset(other.m_offset)
      {
         other.m_buffer = nullptr;
         other.m_capacity = 0;
         other.m_offset = 0;
      }

      ArenaAllocator& operator=(const ArenaAllocator&) = delete;
      ArenaAllocator& operator=(ArenaAllocator&& other) {
         if(this != &other) {
            ::operator delete(m_buffer);
            m_buffer = other.m_buffer;
            m_capacity = other.m_capacity;
            m_offset = other.m_offset;

            other.m_buffer = nullptr;
            other.m_capacity = 0;
            other.m_offset = 0;
         }

         return *this;
      }

      ~ArenaAllocator() { ::operator delete(m_buffer); } // delete without calling any destructors

   public:
      /// @brief only allocates memory, aligned as per T, NO construction involved
      /// @returns address to allocated memory
      template<typename T>
      [[nodiscard]] T* allocate(size_t alignment = alignof(T)) {
         const size_t bytes = sizeof(T);
         const size_t currentAddress = reinterpret_cast<size_t>(m_buffer + m_offset); // so we can use the % operator

         // current % align = how far past the aligned boundary the current address sits
         // (alignment - that) % alignment to get the PADDING not raw value eg. if that = alignment then padding = 0
         const size_t offsetPadding = (alignment - (currentAddress % alignment)) % alignment;

         if(m_offset + offsetPadding + bytes > m_capacity)
            FATAL_ERROR("Tried to allocate more memory than capacity in Arena!");

         m_offset += offsetPadding;
         void* ptr = m_buffer + m_offset;
         m_offset += bytes;

         return static_cast<T*>(ptr);
      }

      /// @brief allocates and constructs T
      template<typename T, typename... Args>
      // && implies: deduce whether each was rvalue or lvalue and preserve that so std::forward works
      [[nodiscard]] T* create(Args&&... args) {
         T* p_T = allocate<T>();
         return ::new (p_T) T(std::forward<Args>(args)...); // construct T and put it at p_T instead of wherever new usually puts it
      }

      void reset() noexcept {
         m_offset = 0;
      }

   public:
      [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }
      [[nodiscard]] size_t used() const noexcept { return m_offset; }
      [[nodiscard]] size_t available() const noexcept { return m_capacity - m_offset; }

   private:
      std::byte* m_buffer = nullptr;
      size_t m_capacity = 0;
      size_t m_offset = 0;
   };
}
