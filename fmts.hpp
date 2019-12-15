#include "fmt/core.h"
#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"

namespace fmt {
	template<>
	struct formatter<c0::ErrorCode> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const c0::ErrorCode &p, FormatContext &ctx) {
			std::string name;
			switch (p) {
			case c0::ErrNoError:
				name = "No error.";
				break;
			case c0::ErrStreamError:
				name = "Stream error.";
				break;
			case c0::ErrEOF:
				name = "EOF";
				break;
			case c0::ErrInvalidInput:
				name = "The input is invalid.";
				break;
			case c0::ErrInvalidIdentifier:
				name = "Identifier is invalid";
				break;
			case c0::ErrIntegerOverflow:
				name = "The integer is too big(int64_t).";
				break;
			case c0::ErrNeedIdentifier:
				name = "Need an identifier here.";
				break;
			case c0::ErrConstantNeedValue:
				name = "The constant need a value to initialize.";
				break;
			case c0::ErrNoSemicolon:
				name = "Zai? Wei shen me bu xie fen hao.";
				break;
			case c0::ErrInvalidVariableDeclaration:
				name = "The declaration is invalid.";
				break;
			case c0::ErrIncompleteExpression:
				name = "The expression is incomplete.";
				break;
			case c0::ErrNotDeclared:
				name = "The variable or constant must be declared before being used.";
				break;
			case c0::ErrAssignToConstant:
				name = "Trying to assign value to a constant.";
				break;
			case c0::ErrDuplicateDeclaration:
				name = "The variable or constant has been declared.";
				break;
			case c0::ErrNotInitialized:
				name = "The variable has not been initialized.";
				break;
			case c0::ErrInvalidAssignment:
				name = "The assignment statement is invalid.";
				break;
			case c0::ErrInvalidPrint:
				name = "The output statement is invalid.";
				break;
			case c0::ErrInvalidFunctionParamType:
				name = "The function parameter type is invalid.";
				break;
			case c0::ErrInvalidFunctionParamCount:
				name = "The function parameter quantity is invalid.";
				break;
			case c0::ErrInvalidType:
				name = "Some expression's type cannot match the variable or const.";
				break;
            case c0::ErrInvalidVariableType:
                name = "Some expression's type cannot match the variable or const.";
                break;
			case c0::ErrNoLeftBrace:
				name = "There should be a left brace.";
				break;
			case c0::ErrNoRightBrace:
				name = "There should be a right brace.";
				break;
			case c0::ErrStatementSequence:
				name = "Something went wrong in a statement sequence.";
				break;
						}
			return format_to(ctx.out(), name);
		}
	};

	template<>
	struct formatter<c0::CompilationError> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const c0::CompilationError &p, FormatContext &ctx) {
			return format_to(ctx.out(), "Line: {} Column: {} Error: {}", p.GetPos().first, p.GetPos().second, p.GetCode());
		}
	};
}

namespace fmt {
	template<>
	struct formatter<c0::Token> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const c0::Token &p, FormatContext &ctx) {
			return format_to(ctx.out(),
				"Line: {} Column: {} Type: {} Value: {}",
				p.GetStartPos().first, p.GetStartPos().second, p.GetType(), p.GetValueString());
		}
	};

	template<>
	struct formatter<c0::TokenType> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const c0::TokenType &p, FormatContext &ctx) {
			std::string name;
			switch (p) {
			case c0::NULL_TOKEN:
				name = "NullToken";
				break;
			case c0::UNSIGNED_INTEGER:
				name = "UnsignedInteger";
				break;
			case c0::IDENTIFIER:
				name = "Identifier";
				break;
			case c0::CONST:
				name = "Const";
				break;
			case c0::VOID:
				name = "Void";
				break;
			case c0::INT:
				name = "Int";
				break;
			case c0::CHAR:
				name = "Char";
				break;
			case c0::DOUBLE:
				name = "Double";
				break;
			case c0::STRUCT:
				name = "Struct";
				break;
			case c0::IF:
				name = "If";
				break;
			case c0::ELSE:
				name = "Else";
				break;
			case c0::SWITCH:
				name = "Switch";
				break;
			case c0::CASE:
				name = "Case";
				break;
			case c0::DEFAULT:
				name = "Default";
				break;
			case c0::WHILE:
				name = "While";
				break;
			case c0::FOR:
				name = "For";
				break;
			case c0::DO:
				name = "Do";
				break;
			case c0::RETURN:
				name = "Return";
				break;
			case c0::BREAK:
				name = "Break";
				break;
			case c0::CONTINUE:
				name = "Continue";
				break;
			case c0::SCAN:
				name = "Scan";
				break;
			case c0::PRINT:
				name = "Print";
				break;
			case c0::HEXADECIMAL:
				name = "Hexadecimal";
				break;
			case c0::PLUS_SIGN:
				name = "PlusSign";
				break;
			case c0::MINUS_SIGN:
				name = "MinusSign";
				break;
			case c0::MULTIPLICATION_SIGN:
				name = "MultiplicationSign";
				break;
			case c0::DIVISION_SIGN:
				name = "DivisionSign";
				break;
			case c0::EQUAL_SIGN:
				name = "EqualSign";
				break;
			case c0::ASSIGN_SIGN:
				name = "AssignSign";
				break;
			case c0::SEMICOLON:
				name = "Semicolon";
				break;
			case c0::LEFT_BRACKET:
				name = "LeftBracket";
				break;
			case c0::RIGHT_BRACKET:
				name = "RightBracket";
				break;
			case c0::LEFT_BRACE:
				name = "LeftBrace";
				break;
			case c0::RIGHT_BRACE:
				name = "RightBrace";
				break;
			case c0::NOT_EQUAL_SIGN:
				name = "NotEqualSign";
				break;
			case c0::GREATER_SIGN:
				name = "GreaterSign";
				break;
			case c0::LESS_SIGN:
				name = "LessSign";
				break;
			case c0::GREATER_EQUAL_SIGN:
				name = "GreaterEqualSign";
				break;
			case c0::LESS_EQUAL_SIGN:
				name = "LessEqualSign";
				break;
			case c0::COMMA:
				name = "Comma";
				break;
			case c0::STRING_VALUE:
				name = "StringValue";
				break;
			case c0::CHAR_VALUE:
				name = "CharValue";
				break;
			case c0::DOUBLE_VALUE:
				name = "DoubleValue";
				break;
			}
			return format_to(ctx.out(), name);
		}
	};
}

namespace fmt {
	template<>
	struct formatter<c0::Operation> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const c0::Operation &p, FormatContext &ctx) {
			std::string name;
			switch (p) {
			case c0::NOP:
				name = "nop";
				break;
			case c0::BIPUSH:
				name = "bipush";
				break;
			case c0::IPUSH:
				name = "ipush";
				break;
			case c0::POP:
				name = "pop";
				break;
			case c0::POP2:
				name = "pop2";
				break;
			case c0::POPN:
				name = "popn";
				break;
			case c0::DUP:
				name = "dup";
				break;
			case c0::DUP2:
				name = "dup2";
				break;
			case c0::LOADC:
				name = "loadc";
				break;
			case c0::LOADA:
				name = "loada";
				break;
			case c0::NEW:
				name = "new";
				break;
			case c0::SNEW:
				name = "snew";
				break;
			case c0::ILOAD:
				name = "iload";
				break;
			case c0::DLOAD:
				name = "dload";
				break;
			case c0::ALOAD:
				name = "aload";
				break;
			case c0::ISTORE:
				name = "istore";
				break;
			case c0::DSTORE:
				name = "dstore";
				break;
			case c0::ASTORE:
				name = "astore";
				break;
			case c0::IASTORE:
				name = "iastore";
				break;
			case c0::DASTORE:
				name = "dastore";
				break;
			case c0::AASTORE:
				name = "aastore";
				break;
			case c0::IADD:
				name = "iadd";
				break;
			case c0::DADD:
				name = "dadd";
				break;
			case c0::ISUB:
				name = "isub";
				break;
			case c0::DSUB:
				name = "dsub";
				break;
			case c0::IMUL:
				name = "imul";
				break;
			case c0::DMUL:
				name = "dmul";
				break;
			case c0::IDIV:
				name = "idiv";
				break;
			case c0::DDIV:
				name = "ddiv";
				break;
			case c0::INEG:
				name = "ineg";
				break;
			case c0::DNEG:
				name = "dneg";
				break;
			case c0::ICMP:
				name = "icmp";
				break;
			case c0::DCMP:
				name = "dcmp";
				break;
			case c0::I2D:
				name = "i2d";
				break;
			case c0::D2I:
				name = "d2i";
				break;
			case c0::I2C:
				name = "i2c";
				break;
			case c0::JMP:
				name = "jmp";
				break;
			case c0::JE:
				name = "je";
				break;
			case c0::JNE:
				name = "jne";
				break;
			case c0::JL:
				name = "jl";
				break;
			case c0::JGE:
				name = "jge";
				break;
			case c0::JG:
				name = "jg";
				break;
			case c0::JLE:
				name = "jle";
				break;
			case c0::CALL:
				name = "call";
				break;
			case c0::RET:
				name = "ret";
				break;
			case c0::IRET:
				name = "iret";
				break;
			case c0::DRET:
				name = "dret";
				break;
			case c0::ARET:
				name = "aret";
				break;
			case c0::IPRINT:
				name = "iprint";
				break;
			case c0::DPRINT:
				name = "dprint";
				break;
			case c0::CPRINT:
				name = "cprint";
				break;
			case c0::SPRINT:
				name = "sprint";
				break;
			case c0::PRINTL:
				name = "printl";
				break;
			case c0::ISCAN:
				name = "iscan";
				break;
			case c0::DSCAN:
				name = "dscan";
				break;
			case c0::CSCAN:
				name = "cscan";
				break;
			case c0::ILL:
				name = "ill";
				break;
			}
			return format_to(ctx.out(), name);
		}
	};
	template<>
	struct formatter<c0::Instruction> {
		template <typename ParseContext>
		constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const c0::Instruction &p, FormatContext &ctx) {
			std::string name;
			switch (p.GetOperation())
			{
			case c0::NOP:
			case c0::POP:
			case c0::POP2:
			case c0::DUP:
			case c0::DUP2:
			case c0::NEW:
			case c0::ILOAD:
			case c0::DLOAD:
			case c0::ALOAD:
			case c0::ISTORE:
			case c0::DSTORE:
			case c0::ASTORE:
			case c0::IASTORE:
			case c0::DASTORE:
			case c0::AASTORE:
			case c0::IADD:
			case c0::DADD:
			case c0::ISUB:
			case c0::DSUB:
			case c0::IMUL:
			case c0::DMUL:
			case c0::IDIV:
			case c0::DDIV:
			case c0::INEG:
			case c0::DNEG:
			case c0::ICMP:
			case c0::DCMP:
			case c0::I2D:
			case c0::D2I:
			case c0::I2C:
			case c0::RET:
			case c0::IRET:
			case c0::DRET:
			case c0::ARET:
			case c0::IPRINT:
			case c0::DPRINT:
			case c0::CPRINT:
			case c0::SPRINT:
			case c0::PRINTL:
			case c0::ISCAN:
			case c0::DSCAN:
			case c0::CSCAN:
			case c0::ILL:
				return format_to(ctx.out(), "{}", p.GetOperation());
			
			case c0::BIPUSH:
			case c0::IPUSH:
			case c0::POPN:
			case c0::LOADC:
			case c0::SNEW:
			case c0::JMP:
			case c0::JE:
			case c0::JNE:
			case c0::JL:
			case c0::JGE:
			case c0::JG:
			case c0::JLE:
            case c0::CALL:
				return format_to(ctx.out(), "{} {}", p.GetOperation(), p.GetX());
			
			case c0::LOADA:
				return format_to(ctx.out(), "{} {} {}", p.GetOperation(), p.GetX(), p.GetOpt());
			}
			return format_to(ctx.out(), "ILL");
		}
	};
}