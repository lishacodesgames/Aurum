#include <pch/Precompiled.h>
#include "mem.h"

namespace mem
{
   void ArenaAllocator::reset() noexcept {
      freeChain(m_head);
      m_head = new Block(m_defaultBlockSize);
   }

   std::size_t ArenaAllocator::capacity() const noexcept {
      std::size_t total = 0;
      for(Block* b = m_head; b; b = b->next)
         total += b->capacity;

      return total;
   }

   std::size_t ArenaAllocator::used() const noexcept {
      std::size_t total = 0;
      for(Block* b = m_head; b; b = b->next)
         total += b->offset;

      return total;
   }

   ArenaAllocator& ArenaAllocator::operator=(ArenaAllocator&& other) noexcept {
      if(this != &other) {
         // free our own chain before overwriting m_head
         freeChain(m_head);

         m_head = other.m_head;
         m_defaultBlockSize = other.m_defaultBlockSize;

         other.m_head = nullptr;
      }

      return *this;
   }

   void ArenaAllocator::freeChain(Block* b) noexcept {
      while(b) {
         Block* next = b->next;
         delete b;
         b = next;
      }
   }

}  // namespace mem
