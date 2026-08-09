#include <pch/Precompiled.h>
#include "Generator.h"

std::string Generator::generate() const {
   std::string assembly;

   assembly += "; macOS x86_64, NASM syntax\n\n";
   assembly += "global _main\n";
   assembly += "_main:\n";

   ast::IntegerLiteral returnValue = *static_cast<ast::IntegerLiteral*>(static_cast<ast::Exit*>(m_root.get())->expression.get());
   assembly += std::format("\tmov eax, {}\n", *returnValue.value.value); /// @note m_root here is of exit type but later it might not be

   assembly += "\tret";

   return assembly;
}
