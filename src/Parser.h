#pragma once
#include "Token.h"
#include "ast.h"
#include "mem.h"

class Parser {
public:
   explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)), m_arena(4 * 1024 * 1024) {} // 4MB for now

   std::expected<ast::Program, std::string> parse();

private:
   std::vector<Token> m_tokens{};
   size_t m_pos = 0;
   mem::ArenaAllocator m_arena;

private:
   Token peek(int offset = 0);

   /** 
    * @brief increments m_pos but returns current Token
    * @param count by how much to increment m_pos
    * @returns CURRENT Token
    */ 
   Token consume(uint32_t count = 1);

private:
   template<AstNode T>
   std::expected<T*, std::string> parse();

   template<VariantNode T>
   std::expected<T, std::string> parse(); // by value because there's no circular dependencies here and we don't want them in the arena
   
   // --- EXPLICIT SPECIALISATIONS ---
   // statements
   template<> std::expected<ast::Statement, std::string> parse<ast::Statement>();
   template<> std::expected<ast::Declaration*, std::string> parse<ast::Declaration>();
   template<> std::expected<ast::Exit*, std::string> parse<ast::Exit>();

   // expressions
   template<> std::expected<ast::Expression, std::string> parse<ast::Expression>();
   template<> std::expected<ast::IntegerLiteral*, std::string> parse<ast::IntegerLiteral>();
   template<> std::expected<ast::Identifier*, std::string> parse<ast::Identifier>();
};
