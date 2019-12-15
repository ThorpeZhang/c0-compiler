#pragma once

#include "error/error.h"
#include "instruction/instruction.h"
#include "tokenizer/token.h"

#include <vector>
#include <optional>
#include <utility>
#include <map>
#include <cstdint>
#include <cstddef> // for std::size_t

namespace c0 {

	class Analyser final {
	private:
		using uint64_t = std::uint64_t;
		using int64_t = std::int64_t;
		using uint32_t = std::uint32_t;
		using int32_t = std::int32_t;
	public:
		Analyser(std::vector<Token> v)
			: _tokens(std::move(v)), _offset(0), _instructions({}), _start_code({}), _current_pos(0, 0), _current_func(0),
			_uninitialized_vars({}), _vars({}), _consts({}), _vars_type({}), _nextTokenIndex({0}), _current_level(-1),
			_funcs({}), _funcs_index_name({}), _nextFunc(0), _consts_offset(0), _runtime_consts({}), _runtime_funcs({}){}
		Analyser(Analyser&&) = delete;
		Analyser(const Analyser&) = delete;
		Analyser& operator=(Analyser) = delete;

		// 接口
		std::pair<std::map<int32_t , std::vector<Instruction>>, std::optional<CompilationError>> Analyse();

        std::vector<Instruction> getStartCode() {return _start_code;}

        std::map<int32_t, std::tuple<std::string, std::string> > getConst() {return _runtime_consts;}

        std::map<int32_t, std::tuple<int32_t, int32_t, int32_t, int32_t> > getFuncs(){return _runtime_funcs;}
	private:
		// 所有的递归子程序

		// <C0-program>
		std::optional<CompilationError> analyseC0Program();
		// 声明变量的过程
		std::optional<CompilationError> analyseDeclaration();
		// <常量声明>
		std::optional<CompilationError> analyseConstantDeclaration();
		// <变量声明>
		std::optional<CompilationError> analyseVariableDeclaration();
		// <函数声明>
		std::optional<CompilationError> analyseFunctionDefinition();
        // <语句序列>
        std::optional<CompilationError> analyseStatementSequence();

        /*
         * <statement> ::=
         * <compound-statement>
         * |<condition-statement>
         * |<loop-statement>
         * |<jump-statement>
         * |<print-statement>
         * |<scan-statement>
         * |<assignment-expression>';'
         * |<function-call>';'
         * |';'
         */

        /*
         * <compound-statement> ::=
         * '{' {<variable-declaration>} <statement-seq> '}'
         */


		// <表达式> == <additive-expression>
		std::optional<CompilationError> analyseExpression(TokenType&);
        // <multiplicative-expression>
        std::optional<CompilationError> analyseMultiExpression(TokenType&);
        // <cast-expression>
        std::optional<CompilationError> analyseCastExpression(TokenType&);
        // <unary-expression>
        std::optional<CompilationError> analyseUnaryExpression(TokenType&);
        // <primary-expression>
        std::optional<CompilationError> analysePrimaryExpression(TokenType&);
        // <赋值语句>
        std::optional<CompilationError> analyseAssignmentStatement();


        std::optional<CompilationError> analyseCompoundStatement();

        std::optional<CompilationError> analyseFunctionCall(TokenType&);

        std::optional<CompilationError> analyseStatement();
        // <condition-statement>
        std::optional<CompilationError> analyseConditionStatement();
        // <labeled-statement>
        ////std::optional<CompilationError> analyseLabeledStatement();

        // <loop-statement>
        ////std::optional<CompilationError> analyseLoopStatement();
        // for
        ////std::optional<CompilationError> analyseForStatement();
        // while
        ////std::optional<CompilationError> analyseWhileStatement();
        // do...while
        ////std::optional<CompilationError> analyseDoWhileStatement();
        // <for-init-statement>
        ////std::optional<CompilationError> analyseForInitStatement();

        // <jump-statement>
        ////std::optional<CompilationError> analyseJumpStatement();

        //IO语句
        // <scan-statement>
        std::optional<CompilationError> analyseScanStatement();
        // <print-statement>
        std::optional<CompilationError> analysePrintStatement();
        // <printable>
        std::optional<CompilationError> analysePrintable();

		// Token 缓冲区相关操作

		// 返回下一个 token
		std::optional<Token> nextToken();
		// 回退一个 token
		void unreadToken();

		// 下面是符号表相关操作

		// helper function
		void _add(const Token&, std::map<std::string, int32_t>&, std::map<std::string, TokenType>&, const TokenType&);
		// 添加变量、常量、未初始化的变量
		void addVariable(const Token&, const TokenType&);
		void addConstant(const Token&, const TokenType&);
		void addUninitializedVariable(const Token&, const TokenType&);
		// 是否在当前层级内被声明过
		// 在声明新变量的时候调用
		bool isDeclared(const std::string&);
		// 是否是可以使用的变量
		bool isUseful(const std::string&);
		// 是否是已初始化的变量
		bool isInitializedVariable(const std::string&);
		// 是否是常量
		bool isConstant(const std::string&);
		// 获得 {变量，常量} 在栈上的层次差和偏移
		std::pair<int32_t, int32_t> getIndex(const std::string&);
		// 获得一个常量或者变量的类型
		TokenType getType(const std::string&);
		// 进入下一个层级的符号表，需要给出初始的栈顶指针，为参数预留位置
		int32_t nextLevel(const int32_t&);
		// 清空当前符号表，并返回上一个层级的符号表下标
		int32_t lastLevel();
        //添加一个运行时的常量，并获取下标
		int32_t addRuntimeConsts(const Token&);
		//向函数表注册一个函数
		//此函数还要完成运行时函数表的注册
		//返回函数参数的slot数
		int32_t addFunc(const Token&, const TokenType&, const std::vector<TokenType>&);
		//函数是否被定义过
		bool isFuncDeclared(const std::string&);
		//获得函数下标
		int32_t getFuncIndex(const std::string &);

	private:
		std::vector<Token> _tokens;
		std::size_t _offset;
        std::map<int32_t, std::vector<Instruction> > _instructions;
		std::vector<Instruction> _start_code;
		std::pair<uint64_t, uint64_t> _current_pos;

		int32_t _current_func;

		/*
		// 为了简单处理，我们直接把符号表耦合在语法分析里
		// 变量                   示例
		// _uninitialized_vars    var a;
		// _vars                  var a=1;
		// _consts                const a=1;
		std::map<std::string, int32_t> _uninitialized_vars;
		std::map<std::string, int32_t> _vars;
		std::map<std::string, int32_t> _consts;
		 */
		// 下一个 token 在栈的偏移
		//int32_t _nextTokenIndex;

		////c0的符号表管理
		//用vector的下标来代表层级
		std::map<int32_t, std::map<std::string, int32_t> > _uninitialized_vars;
		std::map<int32_t, std::map<std::string, int32_t> > _vars;
		std::map<int32_t, std::map<std::string, int32_t> > _consts;
        // 类型管理
        std::map<int32_t, std::map<std::string, TokenType> > _vars_type;
		std::vector<int> _nextTokenIndex;

        //当前的层级
        int32_t _current_level;
        // 初始状态下默认为层级0，因此声明全局变量时不用执行进入下一层级的操作

		//函数表
		// 返回的类型、参数类型列表
		std::map<int32_t, std::pair<TokenType, std::vector<TokenType> > > _funcs;
        std::map<int32_t, std::string> _funcs_index_name;
		int32_t _nextFunc;

		////运行时的表构建
		int32_t _consts_offset;
        std::map<int32_t, std::tuple<std::string, std::string> > _runtime_consts;
		//函数在函数表中的下标、函数名在常量表中的下标、参数占空间的大小、函数层级
		std::map<int32_t, std::tuple<int32_t, int32_t, int32_t, int32_t> > _runtime_funcs;
	};
}
