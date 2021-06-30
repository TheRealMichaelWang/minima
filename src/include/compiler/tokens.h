#ifndef TOKENS_H
#define TOKENS_H

#include "../runtime/operators.h"
#include "../runtime/value.h"

#define IS_VALUE_TOK(TOK) (TOK.type < 12 && TOK.type >= 0)
#define IS_STATMENT_TOK(TOK) (TOK.type - 9 < 10)

struct token {
	enum token_type {
		TOK_PRIMATIVE, //values
		TOK_STR,
		TOK_REF,
		TOK_IDENTIFIER,
		TOK_OPEN_BRACE,
		TOK_NEW,
		TOK_OPEN_PAREN,
		TOK_ALLOC,
		TOK_BINARY_OP,

		TOK_UNARY_OP,  //statments/values
		TOK_GOTO,
		TOK_EXTERN,

		TOK_SET, //statments
		TOK_IF,
		TOK_WHILE,
		TOK_PROC,
		TOK_RECORD,
		TOK_RETURN,
		TOK_INCLUDE,

		TOK_ELSE,
		TOK_ELIF,
		TOK_TO,
		TOK_AS,
		TOK_EXTEND,
		TOK_REMARK,
		TOK_CLOSE_PAREN,
		TOK_CLOSE_BRACE,
		TOK_OPEN_BRACKET,
		TOK_CLOSE_BRACKET,
		TOK_COMMA,
		TOK_PERIOD,
		TOK_END,
		TOK_ERROR
	} type;

	union token_type_payload
	{
		enum binary_operator bin_op;
		enum unary_operator uni_op;

		struct value primative;

		uint64_t identifier;
	} payload;
};

#endif // !TOKENS_H