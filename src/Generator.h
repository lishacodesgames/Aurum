#pragma once
#include "Nodes.h"

class Generator {
public:
   explicit Generator(ast::Node root) : m_root(std::move(root)) {}

   std::string generate() const;

private:
   const ast::Node m_root;
};
