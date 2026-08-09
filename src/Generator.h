#pragma once
#include "Nodes.h"

class Generator {
public:
   explicit Generator(std::unique_ptr<ast::Node> root) : m_root(std::move(root)) {}

   std::string generate() const;

private:
   const std::unique_ptr<ast::Node> m_root;
};
