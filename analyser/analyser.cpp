#include "analyser.h"

#include <climits>
#include <sstream>

int count = 0;

namespace c0 {
	std::pair<std::map<int32_t, std::vector<Instruction>>, std::optional<CompilationError>> Analyser::Analyse() {
        auto err = analyseC0Program();
        if (err.has_value())
			return std::make_pair(std::map<int32_t, std::vector<Instruction>>(), err);
		else
			return std::make_pair(_instructions, std::optional<CompilationError>());
	}

	// <C0-program> ::=
    //    {<variable-declaration>}{<function-definition>}
    std::optional<CompilationError> Analyser::analyseC0Program() {
        nextLevel(0);
        auto err = analyseDeclaration();
	    if(err.has_value())
	        return err;

	    err = analyseFunctionDefinition();
	    if(err.has_value())
	        return err;

	    return {};
	}

	// <function-definition> ::=
    //    <type-specifier><identifier><parameter-clause><compound-statement>
    //<simple-type-specifier>  ::= 'void'|'int'|'char'|'double'
    // <parameter-clause> ::=
    //    '(' [<parameter-declaration-list>] ')'
    //<parameter-declaration-list> ::=
    //    <parameter-declaration>{','<parameter-declaration>}
    //<parameter-declaration> ::=
    //    [<const-qualifier>]<type-specifier><identifier>
    std::optional<CompilationError> Analyser::analyseFunctionDefinition() {
	    // 全局变量的声明产生的指令是保存在函数0的指令栈里面的，因此需要转移，并且清空函数指令栈
	    auto itr = _instructions[_current_func].begin();
	    while(itr != _instructions[_current_func].end()) {
	        _start_code.emplace_back(*itr);
	        itr++;
	    }
	    _instructions[_current_func].clear();


	    while(true) {
            // 获取返回类型
            auto next = nextToken();
            if(!next.has_value())
                return {};

            auto retType = next.value().GetType();
            if(retType != TokenType::INT && retType != TokenType::DOUBLE && retType != TokenType::CHAR && retType != TokenType::VOID)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

            // 获取函数名
            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            if(isFuncDeclared(next.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
            auto funcName = next.value();

            //分析参数列表
            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);

            nextLevel(0);

            std::vector<TokenType> paramTypes;
            while(true) {
                ////读取const或者类型
                bool _isconst = false;
                next = nextToken();
                if(!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

                if(next.value().GetType() == TokenType::RIGHT_BRACKET)
                    break;

                if(next.value().GetType() == TokenType::CONST) {
                    _isconst = true;
                    next = nextToken();
                    if(!next.has_value())
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
                }

                auto type = next.value().GetType();
                if(type != TokenType::INT && type != TokenType::DOUBLE && type != TokenType::CHAR)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

                paramTypes.emplace_back(type);

                ////获取标识符
                next = nextToken();
                if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

                if(_isconst)
                    addConstant(next.value(), type);
                else
                    addVariable(next.value(), type);

                next = nextToken();
                if(!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);

                if(next.value().GetType() == TokenType::COMMA)
                    continue;
                else if(next.value().GetType() == TokenType::RIGHT_BRACKET)
                    break;
                else
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);
            }

            ////添加函数
            addFunc(funcName, retType, paramTypes);

            ////开始分析函数体
            auto err = analyseCompoundStatement();
            if(err.has_value())
                return err;

            //// 函数体分析完成，返回上一层符号表
            lastLevel();
	    }
        return {};
	}

    //<compound-statement> ::=
    //    '{' {<variable-declaration>} <statement-seq> '}'
    std::optional<CompilationError> Analyser::analyseCompoundStatement() {

        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);

        auto err = analyseDeclaration();
        if(err.has_value())
            return err;

        err = analyseStatementSequence();
        if(err.has_value())
            return err;

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACE)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);


        return {};
	}

    //<statement-seq> ::=
    //	{<statement>}
    //<statement> ::=
    //     <compound-statement>
    //    |<condition-statement>
    ////    |<loop-statement>
    ////    |<jump-statement>
    //    |<print-statement>
    //    |<scan-statement>
    //    |<assignment-expression>';'
    //    |<function-call>';'
    //    |';'
    std::optional<CompilationError> Analyser::analyseStatement() {
        auto next = nextToken();
        if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrStatementSequence);
        std::optional<CompilationError> err;
        auto type = next.value().GetType();
        if(type == TokenType::LEFT_BRACE) {
            unreadToken();
            int32_t sp = _nextTokenIndex[_current_level];
            nextLevel(sp);
            err = analyseCompoundStatement();
            lastLevel();
        }
        else if(type == TokenType::PRINT) {
            unreadToken();
            err = analysePrintStatement();
        }
        else if(type == TokenType::SCAN) {
            unreadToken();
            err = analyseScanStatement();
        }
        else if(type == TokenType::RETURN) {
            unreadToken();
            err = analyseRetStatement();
        }
        else if(type == TokenType::SWITCH) {
            unreadToken();
            err = analyseSwitchStatement();
        }
        else if(type == TokenType::IDENTIFIER) {
            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrStatementSequence);

            unreadToken();
            unreadToken();
            type = next.value().GetType();
            TokenType typeTest;
            if(type == TokenType::LEFT_BRACKET) {
                err = analyseFunctionCall(typeTest);
                if(err.has_value())
                    return err;

                if(typeTest == TokenType::DOUBLE)
                    _instructions[_current_func].emplace_back(Operation::POP2);
                else if(typeTest != TokenType::VOID)
                    _instructions[_current_func].emplace_back(Operation::POP);

            }
            else if(type == TokenType::ASSIGN_SIGN)
                err = analyseAssignmentStatement();
            else
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrStatementSequence);

            if(err.has_value())
                return err;

            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

            return {};
        }
        else if(type == TokenType::FOR) {
            unreadToken();
            err = analyseForStatement();
        }
        else if(type == TokenType::IF) {
            unreadToken();
            err = analyseIfStatement();
        }
        else if(type == TokenType::BREAK)
            err = analyseBreakStatement();
        else if(type == TokenType::CONTINUE)
            err = analyseContinueStatement();
        else if(type == TokenType::WHILE) {
            unreadToken();
            err = analyseWhileStatement();
        }
        else if(type == TokenType::DO) {
            unreadToken();
            err = analyseDoWhileStatement();
        }
        else if(type == TokenType::SEMICOLON)
            return {};
        else {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrStatementSequence);
        }

        if(err.has_value())
            return err;

        return {};
	}

    std::optional<CompilationError> Analyser::analyseStatementSequence() {

	    while(true) {
            auto next = nextToken();
            if(!next.has_value())
                return {};
            unreadToken();
            if(next.value().GetType() == TokenType::RIGHT_BRACE)
                return {};

            auto err = analyseStatement();
            if(err.has_value())
                return err;
	    }
	}

	// <condition> ::=
    //     <expression>[<relational-operator><expression>]
    // <relational-operator>     ::= '<' | '<=' | '>' | '>=' | '!=' | '=='
    // <condition-statement> ::=
    //     'if' '(' <condition> ')' <statement> ['else' <statement>]
    //    |'switch' '(' <expression> ')' '{' {<labeled-statement>} '}'
    // je：  0   value是0
    // jne： 1   value不是0
    // jl：  2   value是负数
    // jge： 3   value不是负数
    // jg：  4   value是正数
    // jle： 5   value不是正数
    // Condition返回的是不满足条件的_option
    std::optional<CompilationError> Analyser::analyseCondition(int32_t & _option) {
        auto next = nextToken();
        if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
        unreadToken();

        TokenType typeTest, type;
        auto err = analyseExpression(type);
        if(err.has_value())
            return err;
        if(type == TokenType::VOID)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
        _option = 0;

        next = nextToken();
        if(!next.has_value())
            return {};

        switch(next.value().GetType()) {
            case TokenType ::LESS_SIGN :
                _option = 3;
                break;
            case TokenType ::LESS_EQUAL_SIGN:
                _option = 4;
                break;
            case TokenType ::GREATER_SIGN:
                _option = 5;
                break;
            case TokenType ::GREATER_EQUAL_SIGN:
                _option = 2;
                break;
            case TokenType ::EQUAL_SIGN:
                _option = 1;
                break;
            case TokenType ::NOT_EQUAL_SIGN:
                _option = 0;
                break;
            default:
                unreadToken();
                return {};
        }

        err = analyseExpression(typeTest);
        if(err.has_value())
            return err;
        if(typeTest == TokenType::VOID)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

        if(type == TokenType::DOUBLE) {
            if(typeTest == TokenType::INT || typeTest == TokenType::CHAR)
                _instructions[_current_func].emplace_back(Operation::I2D);
            _instructions[_current_func].emplace_back(Operation::DCMP);
        }
        else if(type == TokenType::INT || type == TokenType::CHAR) {
            if(typeTest == TokenType::DOUBLE)
                _instructions[_current_func].emplace_back(Operation::D2I);
            _instructions[_current_func].emplace_back(Operation::ICMP);

        }

        return {};
	}
    std::optional<CompilationError> Analyser::analyseIfStatement() {
	    ////进入此函数之前已经读到if
	    auto next = nextToken();
	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	        unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);
	    }

	    int32_t _option;
	    auto err = analyseCondition(_option);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);
        }

        ////如何实现跳转？
        Operation _op;
        switch(_option) {
            case 0:
              _op = Operation ::JE;
              break;
            case 1:
                _op = Operation ::JNE;
                break;
            case 2:
                _op = Operation ::JL;
                break;
            case 3:
                _op = Operation ::JGE;
                break;
            case 4:
                _op = Operation ::JG;
                break;
            case 5:
                _op = Operation ::JLE;
                break;
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
        }

        //  不满足条件，跳转到if后面的句子的末尾，如果有，else，则跳转到else的开头
        int32_t _jmp = _instructions[_current_func].size();
        _instructions[_current_func].emplace_back(_op);

        err = analyseStatement();
        if(err.has_value()){
            return err;
        }

        _instructions[_current_func][_jmp].set_X(_instructions[_current_func].size());
        _instructions[_current_func].emplace_back(Operation::NOP);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::ELSE) {
            unreadToken();
            return {};
        }
        // 跳转到else句子的末尾
        _instructions[_current_func].pop_back();
        int32_t _jmp2 = _instructions[_current_func].size();
        _instructions[_current_func].emplace_back(Operation::JMP);
        //跳转到else开头
        _instructions[_current_func][_jmp].set_X(_instructions[_current_func].size());

        err = analyseStatement();
        if(err.has_value()){
            return err;
        }

        if(_instructions[_current_func].back().GetOperation() == Operation::NOP)
            _instructions[_current_func][_jmp2].set_X(_instructions[_current_func].size() - 1);
        else {
            _instructions[_current_func][_jmp2].set_X(_instructions[_current_func].size());
            _instructions[_current_func].emplace_back(Operation::NOP);
        }


        return {};
	}

	// 'while' '(' <condition> ')' <statement>
	// <jump-statement> ::=
    //     'break' ';'
    //    |'continue' ';'
    std::optional<CompilationError> Analyser::analyseBreakStatement() {
	    //// 进入此函数之前已经读到break，此处不再把读走的break放回来
	    if(_current_loop < 0)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrBreak);

	    auto next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
	        unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
	    }

	    breaks[_current_loop].emplace_back(_instructions[_current_func].size());
	    _instructions[_current_func].emplace_back(Operation::JMP);


	    return {};
	}
    std::optional<CompilationError> Analyser::analyseContinueStatement() {
        //// 进入此函数之前已经读到break，此处不再把读走的continue放回来
        if(_current_loop < 0)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrContinue);

        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
        }

        continues[_current_loop].emplace_back(_instructions[_current_func].size());
        _instructions[_current_func].emplace_back(Operation::JMP);


        return {};
    }
    std::optional<CompilationError> Analyser::analyseWhileStatement() {
	    //// 进入此函数前已经读到了while
	    auto next = nextToken();

	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	        unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);
	    }

	    ////为分析循环体作准备
	    _current_loop++;
	    int32_t beginOfWhile = _instructions[_current_func].size();

	    int32_t _option;
	    auto err = analyseCondition(_option);
	    if(err.has_value())
	        return err;

	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);
        }


        Operation _op;
        switch(_option) {
            case 0:
                _op = Operation ::JE;
                break;
            case 1:
                _op = Operation ::JNE;
                break;
            case 2:
                _op = Operation ::JL;
                break;
            case 3:
                _op = Operation ::JGE;
                break;
            case 4:
                _op = Operation ::JG;
                break;
            case 5:
                _op = Operation ::JLE;
                break;
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
        }

        int32_t _jmp =_instructions[_current_func].size();
        _instructions[_current_func].emplace_back(_op);

        err = analyseStatement();
        if(err.has_value())
            return err;

        ////补全continue的参数
        int32_t continueOfWhile = _instructions[_current_func].size();
        auto itr = continues[_current_loop].begin();
        while(itr != continues[_current_loop].end()) {
            _instructions[_current_func][*itr].set_X(continueOfWhile);
            itr++;
        }
        continues[_current_loop].clear();
        continues.erase(_current_loop);

        _instructions[_current_func].emplace_back(Operation::JMP, beginOfWhile);

        ////补全break的参数
        int32_t breakOfWhile = _instructions[_current_func].size();
        itr = breaks[_current_loop].begin();
        while(itr != breaks[_current_loop].end()) {
            _instructions[_current_func][*itr].set_X(breakOfWhile);
            itr++;
        }
        breaks[_current_loop].clear();
        breaks.erase(_current_loop);

        _instructions[_current_func][_jmp].set_X(breakOfWhile);

        _instructions[_current_func].emplace_back(Operation::NOP);

        _current_loop--;
        return {};
	}

	// 'for' '('<for-init-statement> [<condition>]';' [<for-update-expression>]')' <statement>
	// <for-init-statement> ::=
    //    [<assignment-expression>{','<assignment-expression>}]';'
    // <for-update-expression> ::=
    //    (<assignment-expression>|<function-call>){','(<assignment-expression>|<function-call>)}
    std::optional<CompilationError> Analyser::analyseForStatement() {
	    ////进入此函数前已经读到了for
	    auto next = nextToken();
	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	        unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);
	    }
        next = nextToken();
        if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        if(next.value().GetType() != TokenType::SEMICOLON) {
            // 开始分析赋值语句
            unreadToken();
            while(true) {
                auto err = analyseAssignmentStatement();
                if(err.has_value())
                    return err;

                next = nextToken();
                if(!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);

                if(next.value().GetType() == TokenType::SEMICOLON)
                    break;
                else if(next.value().GetType() == TokenType::COMMA)
                    continue;
                else
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLoop);
            }
        }

	    int32_t beginOfFor = _instructions[_current_func].size();
        next = nextToken();
        if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        int32_t _option;
        // je：  0   value是0
        // jne： 1   value不是0
        // jl：  2   value是负数
        // jge： 3   value不是负数
        // jg：  4   value是正数
        // jle： 5   value不是正数
        // Condition返回的是不满足条件的_option
        if(next.value().GetType() == TokenType::SEMICOLON) {
            _instructions[_current_func].emplace_back(Operation::BIPUSH, 1);
            _option = 0;
        }
        else {
            unreadToken();
            auto err = analyseCondition(_option);
            if(err.has_value())
                return err;

            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON){
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
            }
        }
        Operation _op;
        switch (_option) {
            case 0:
                _op = Operation ::JE;
                break;
            case 1:
                _op = Operation ::JNE;
                break;
            case 2:
                _op = Operation ::JL;
                break;
            case 3:
                _op = Operation ::JGE;
                break;
            case 4:
                _op = Operation ::JG;
                break;
            case 5:
                _op = Operation ::JLE;
                break;
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLoop);

        }

        int32_t _jmp1 = _instructions[_current_func].size();
        _instructions[_current_func].emplace_back(_op);

        // 分析update语句

        int32_t continueOfFor = _instructions[_current_func].size();
        next = nextToken();
        if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);

        if(next.value().GetType() != TokenType::RIGHT_BRACKET) {
            unreadToken();
            while(true) {
                next = nextToken();
                if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

                next = nextToken();
                if(!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLoop);

                unreadToken();
                unreadToken();
                std::optional<CompilationError> err;
                TokenType typeTest;
                if(next.value().GetType() == TokenType::ASSIGN_SIGN)
                    err = analyseAssignmentStatement();
                else if(next.value().GetType() == TokenType::LEFT_BRACKET)
                    err = analyseFunctionCall(typeTest);
                else
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLoop);

                if(err.has_value())
                    return err;

                next = nextToken();
                if(!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);
                if(next.value().GetType() == TokenType::COMMA)
                    continue;
                else if(next.value().GetType() == TokenType::RIGHT_BRACKET)
                    break;
                else
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLoop);
            }
        }

        _current_loop++;

        auto err = analyseStatement();
        if(err.has_value())
            return err;

        _instructions[_current_func].emplace_back(Operation::JMP, beginOfFor);

        int32_t breakOfFor = _instructions[_current_func].size();
        _instructions[_current_func][_jmp1].set_X(breakOfFor);

        auto itr = breaks[_current_loop].begin();
        while(itr != breaks[_current_loop].end()) {
            _instructions[_current_func][*itr].set_X(breakOfFor);
            itr++;
        }
        breaks[_current_loop].clear();
        breaks.erase(_current_loop);

        itr = continues[_current_loop].begin();
        while(itr != continues[_current_loop].end()) {
            _instructions[_current_func][*itr].set_X(continueOfFor);
            itr++;
        }
        continues[_current_loop].clear();
        continues.erase(_current_loop);

        _current_loop--;
        return {};
	}

	// 'do' <statement> 'while' '(' <condition> ')' ';'
    std::optional<CompilationError> Analyser::analyseDoWhileStatement() {
	    //// 进入此函数前已经读到了do
	    auto next = nextToken();

	    _current_loop++;
	    int32_t beginOfDoWhile = _instructions[_current_func].size();

	    auto err = analyseStatement();
	    if(err.has_value()) {
            return err;
	    }

        ////补全continue的参数
        int32_t continueOfDoWhile = _instructions[_current_func].size();
        auto itr = continues[_current_loop].begin();
        while(itr != continues[_current_loop].end()) {
            _instructions[_current_func][*itr].set_X(continueOfDoWhile);
            itr++;
        }
        continues[_current_loop].clear();
        continues.erase(_current_loop);

        _instructions[_current_func].emplace_back(Operation::NOP);

	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::WHILE) {
	        unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLoop);
	    }
        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);
        }

	    int32_t _option;
	    err = analyseCondition(_option);
	    if(err.has_value())
	        return err;

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);
        }

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
        }
	    // je：  0   value是0
        // jne： 1   value不是0
        // jl：  2   value是负数
        // jge： 3   value不是负数
        // jg：  4   value是正数
        // jle： 5   value不是正数
        // Condition返回的是不满足条件的_option
        // 但是此处期望满足条件的_option
        Operation _op;
        switch(_option) {
            case 0:
                _op = Operation ::JNE;
                break;
            case 1:
                _op = Operation ::JE;
                break;
            case 2:
                _op = Operation ::JGE;
                break;
            case 3:
                _op = Operation ::JL;
                break;
            case 4:
                _op = Operation ::JLE;
                break;
            case 5:
                _op = Operation ::JG;
                break;
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
        }

        _instructions[_current_func].emplace_back(_op, beginOfDoWhile);

        ////补全break的参数
        int32_t breakOfWhile = _instructions[_current_func].size();
        itr = breaks[_current_loop].begin();
        while(itr != breaks[_current_loop].end()) {
            _instructions[_current_func][*itr].set_X(breakOfWhile);
            itr++;
        }
        breaks[_current_loop].clear();
        breaks.erase(_current_loop);

        _instructions[_current_func].emplace_back(Operation::NOP);

	    _current_loop--;
	    return {};
	}

	// 'switch' '(' <expression> ')' '{' {<labeled-statement>} '}'
	// <labeled-statement> ::=
    //     'case' (<integer-literal>|<char-literal>) ':' <statement>
    //    |'default' ':' <statement>
    std::optional<CompilationError> Analyser::analyseSwitchStatement() {
	    //// 进入此函数前已经读到了switch
	    auto next = nextToken();
	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
	        unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);
	    }

	    TokenType typeTest;
	    auto err = analyseExpression(typeTest);
	    if(err.has_value())
	        return err;
	    if(typeTest != TokenType::INT && typeTest != TokenType::CHAR)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidSwitchType);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);
        }

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACE) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);
        }

        _current_loop++;
        std::set<std::string> cases;
        std::vector<int32_t > ends;

        while(true) {
            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedCase);

            if(next.value().GetType() == TokenType::DEFAULT) {
                next = nextToken();
                if(!next.has_value() || next.value().GetType() != TokenType::COLON_SIGN)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedColon);

                auto itr = ends.begin();
                while(itr != ends.end()) {
                    _instructions[_current_func][*itr].set_X(_instructions[_current_func].size());
                    itr++;
                }
                ends.clear();

                err = analyseStatement();
                if(err.has_value())
                    return err;

                break;
            }

            if(next.value().GetType() != TokenType::CASE) {
                unreadToken();
                auto itr = ends.begin();
                while(itr != ends.end()) {
                    _instructions[_current_func][*itr].set_X(_instructions[_current_func].size());
                    itr++;
                }
                ends.clear();
                break;
            }

            next = nextToken();
            if(!next.has_value() ||
                (next.value().GetType() != TokenType::UNSIGNED_INTEGER
                        && next.value().GetType() != TokenType::HEXADECIMAL
                        && next.value().GetType() != TokenType::CHAR_VALUE)) {
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidCaseType);
            }

            auto type = next.value().GetType();

            std::string str = next.value().GetValueString();
            if(cases.find(str) != cases.end())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDupCase);

            _instructions[_current_func].emplace_back(Operation::DUP);
            cases.insert(str);
            if(type == TokenType::HEXADECIMAL) {
                int32_t index = addRuntimeConsts(next.value());
                _instructions[_current_func].emplace_back(Operation::LOADC, index);
            }
            else if(type == TokenType::UNSIGNED_INTEGER) {
                int32_t val = std::any_cast<int32_t>(next.value().GetValue());
                _instructions[_current_func].emplace_back(Operation::IPUSH, val);
            }
            else {
                char val = std::any_cast<char>(next.value().GetValue());
                _instructions[_current_func].emplace_back(Operation::BIPUSH, val);
            }

            _instructions[_current_func].emplace_back(Operation::ICMP);
            int32_t _jmp = _instructions[_current_func].size();
            _instructions[_current_func].emplace_back(Operation::JNE);

            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::COLON_SIGN)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedColon);

            auto itr = ends.begin();
            while(itr != ends.end()) {
                _instructions[_current_func][*itr].set_X(_instructions[_current_func].size());
                itr++;
            }
            ends.clear();

            err = analyseStatement();
            if(err.has_value())
                return err;

            ends.emplace_back(_instructions[_current_func].size());
            _instructions[_current_func].emplace_back(Operation::JMP);

            _instructions[_current_func][_jmp].set_X(_instructions[_current_func].size());

        }

        if(_current_loop == 1 && !continues[_current_loop].empty())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrContinue);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACE) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);
        }

        auto itr = breaks[_current_loop].begin();
        while(itr != breaks[_current_loop].end()) {
            _instructions[_current_func][*itr].set_X(_instructions[_current_func].size());
            itr++;
        }
        breaks[_current_loop].clear();
        breaks.erase(_current_loop);

        itr = continues[_current_loop].begin();
        while(itr != continues[_current_loop].end()) {
            continues[_current_loop - 1].emplace_back(*itr);
            itr++;
        }
        continues[_current_loop].clear();
        continues.erase(_current_loop);

        _instructions[_current_func].emplace_back(Operation::NOP);
        _current_loop--;
        return {};
	}


	// <print-statement> ::=
    //    'print' '(' [<printable-list>] ')' ';'
    // <printable-list>  ::=
    //    <printable> {',' <printable>}
    // <printable> ::=
    //    <expression> | <string-literal>
    std::optional<CompilationError> Analyser::analysePrintStatement() {
	    //// 进入此函数之前已经预读到print
	    auto next = nextToken();
	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

	    while(true) {
            auto err = analysePrintable();
            if(err.has_value())
                return err;

            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

            if(next.value().GetType() == TokenType::COMMA)
                continue;
            else if(next.value().GetType() == TokenType::RIGHT_BRACKET)
                break;
            else
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);
	    }

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);


        _instructions[_current_func].emplace_back(Operation::PRINTL);
        return {};
	}

    // <printable> ::=
    //    <expression> | <string-literal>
    std::optional<CompilationError> Analyser::analysePrintable() {
	    auto next = nextToken();
	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

	    if(next.value().GetType() == TokenType::STRING_VALUE) {
	        auto index = addRuntimeConsts(next.value());
	        _instructions[_current_func].emplace_back(Operation::LOADC, index);
            _instructions[_current_func].emplace_back(Operation::SPRINT);
	    }
	    else if(next.value().GetType() == TokenType::CHAR_VALUE) {
	        char ch = std::any_cast<char>(next.value().GetValue());
            _instructions[_current_func].emplace_back(Operation::BIPUSH, ch);
            _instructions[_current_func].emplace_back(Operation::CPRINT);
	    }
	    else {
	        unreadToken();
	        TokenType typeTest;
	        auto err = analyseExpression(typeTest);
	        if(err.has_value())
	            return err;

	        if(typeTest == TokenType::DOUBLE)
                _instructions[_current_func].emplace_back(Operation::DPRINT);
            else if(typeTest == TokenType::CHAR)
                _instructions[_current_func].emplace_back(Operation::CPRINT);
	        else if(typeTest == TokenType::INT)
                _instructions[_current_func].emplace_back(Operation::IPRINT);
            else if(typeTest == TokenType::VOID)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);
	    }
        _instructions[_current_func].emplace_back(Operation::BIPUSH, ' ');
        _instructions[_current_func].emplace_back(Operation::CPRINT);

	    return {};
	}

	// <scan-statement> ::=
    //    'scan' '(' <identifier> ')' ';'
    std::optional<CompilationError> Analyser::analyseScanStatement() {
	    // 已经读到了scan
        auto next = nextToken();

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidInput);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

        auto tk = next.value();
        if(!isUseful(tk.GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
        if(isConstant(tk.GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);

        auto type = getType(tk.GetValueString());
        auto index = getIndex(tk.GetValueString());
        _instructions[_current_func].emplace_back(Operation::LOADA, index.first, index.second);

        if(type == TokenType::INT) {
            _instructions[_current_func].emplace_back(Operation::ISCAN);
            _instructions[_current_func].emplace_back(Operation::ISTORE);
        }
        else if(type == TokenType::DOUBLE) {
            _instructions[_current_func].emplace_back(Operation::DSCAN);
            _instructions[_current_func].emplace_back(Operation::DSTORE);
        }
        else if(type == TokenType::CHAR) {
            _instructions[_current_func].emplace_back(Operation::CSCAN);
            _instructions[_current_func].emplace_back(Operation::ISTORE);
        }
        else
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidInput);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        if(!isInitializedVariable(tk.GetValueString())) {
            int32_t level = _current_level;
            while(level >= 0) {
                if(_uninitialized_vars[level].find(tk.GetValueString()) != _uninitialized_vars[level].end()) {
                    _vars[level][tk.GetValueString()] = _uninitialized_vars[level][tk.GetValueString()];
                    _uninitialized_vars[level].erase(tk.GetValueString());
                    break;
                }
                level --;
            }
        }

        return {};

	}

	// <variable-declaration> ::=
    //    [<const-qualifier>]<type-specifier><init-declarator-list>';'
    std::optional<CompilationError> Analyser::analyseDeclaration() {
        while(true) {
            auto next = nextToken();
            if(!next.has_value())
                return {};

            auto type = next.value().GetType();
            switch(type) {
                // 当预读到普通类型的时候，需要判断是变量的声明还是函数的定义
                // 再次预读一个token，期望其类型是IDENTIFIER，但并不做检查，因为检查可以留给子函数进行
                // 再预读一个token，这个要检查类型
                // 如果类型是左括号，说明是函数定义，应返回
                // 否则调用变量声明子函数，其余检查在子函数中进行
                // 预读的总共三个token都要返回
                case TokenType ::DOUBLE:
                case TokenType ::INT:
                case TokenType ::CHAR: {
                    next = nextToken();
                    if(!next.has_value())
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

                    next = nextToken();
                    if(!next.has_value())
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

                    unreadToken();
                    unreadToken();
                    unreadToken();

                    if(next.value().GetType() == TokenType::LEFT_BRACKET)
                        return {};

                    auto err = analyseVariableDeclaration();
                    if(err.has_value())
                        return err;
                    break;
                }

                // 只要遇到void，说明就已经到达定义函数的部分了
                // 变量的类型不可能为void
                case TokenType ::VOID:
                    unreadToken();
                    return {};

                case TokenType ::CONST:{
                    auto err = analyseConstantDeclaration();
                    if(err.has_value())
                        return err;
                    break;
                }

                default:
                    unreadToken();
                    return {};

            }
        }
	}

	// 常量和变量指令不一样，因此分为两个子函数
	// 每次只执行一个分号分开的语句

	// <variable-declaration> ::=
    //    [<const-qualifier>]<type-specifier><init-declarator-list>';'
    // <init-declarator-list> ::=
    //    <init-declarator>{','<init-declarator>}
    // <init-declarator> ::=
    //    <identifier>[<initializer>]
    // <initializer> ::=
    //    '='<expression>
    std::optional<CompilationError> Analyser::analyseVariableDeclaration() {
        //进入到此函数之前一定读到了类型
	    // 类型只能为int、double、char
        auto next = nextToken();
        auto type = next.value().GetType();

        while(true) {
            // 读标识符
            // 只能通过读取分号跳出此循环
            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            }

            if(isDeclared(next.value().GetValueString())) {
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
            }
            auto identifier = next.value();

            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
            // '='
            if(next.value().GetType() == TokenType::ASSIGN_SIGN) {

                if(type == TokenType::DOUBLE)
                    _instructions[_current_func].emplace_back(Operation::SNEW, 2);
                else
                    _instructions[_current_func].emplace_back(Operation::SNEW, 1);



                addVariable(identifier, type);
                auto index = getIndex(identifier.GetValueString());

                _instructions[_current_func].emplace_back(Operation::LOADA, index.first, index.second);

                //// 需要在此处调用表达式分析子程序
                TokenType typeTest;
                auto err = analyseExpression(typeTest);
                if(err.has_value())
                    return err;


                if(type != typeTest) {
                    if(type == TokenType::DOUBLE && (typeTest == TokenType::INT || typeTest == TokenType::CHAR))
                        _instructions[_current_func].emplace_back(Operation::I2D);
                    else if(type == TokenType::INT && typeTest == TokenType::DOUBLE)
                        _instructions[_current_func].emplace_back(Operation::D2I);
                    else if(type == TokenType::CHAR) {
                        if(typeTest == TokenType::DOUBLE) {
                            _instructions[_current_func].emplace_back(Operation::D2I);
                            _instructions[_current_func].emplace_back(Operation::I2C);
                        }
                        else if(typeTest == TokenType::INT)
                            _instructions[_current_func].emplace_back(Operation::I2C);
                    }

                    else
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

                }

                if(type == TokenType::DOUBLE)
                    _instructions[_current_func].emplace_back(Operation::DSTORE);
                else
                    _instructions[_current_func].emplace_back(Operation::ISTORE);



                next = nextToken();
                if(!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);

                if(next.value().GetType() == TokenType::COMMA)
                    continue;
                else if(next.value().GetType() == TokenType::SEMICOLON)
                    return {};
                else {
                    unreadToken();
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
                }

            }
            else if(next.value().GetType() == TokenType::COMMA || next.value().GetType() == TokenType::SEMICOLON) {
                addUninitializedVariable(identifier, type);
                //auto index = getIndex(identifier.GetValueString());
                if(type == TokenType::INT || type == TokenType::CHAR) {
                    _instructions[_current_func].emplace_back(Operation::SNEW, 1);
                    //_instructions[_current_func].emplace_back(Operation::LOADA, index.first, index.second);
                    //_instructions[_current_func].emplace_back(Operation::IPUSH, 0);
                    //_instructions[_current_func].emplace_back(Operation::ISTORE);
                }

                    //

                else if(type == TokenType::DOUBLE) {
                    _instructions[_current_func].emplace_back(Operation::SNEW, 2);
                }

                if(next.value().GetType() == TokenType::SEMICOLON)
                    return {};
            }
            else {
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
            }
        }

        return {};
	}

    std::optional<CompilationError> Analyser::analyseConstantDeclaration() {
	    //进入到此函数之前一定读到了const
	    // const已经被读走了
        // 类型只能为int、double、char
	    auto next = nextToken();

	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

	    auto type = next.value().GetType();
	    if(type != TokenType::INT && type != TokenType::DOUBLE && type != TokenType::CHAR) {
	        unreadToken();
	        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableType);
	    }

	    while(true) {
            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

            if(isDeclared(next.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);

            if(type == TokenType::DOUBLE)
                _instructions[_current_func].emplace_back(Operation::SNEW, 2);
            else
                _instructions[_current_func].emplace_back(Operation::SNEW, 1);

            auto identifier = next.value();


            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::ASSIGN_SIGN)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);


            addConstant(identifier, type);
            auto index = getIndex(identifier.GetValueString());
            _instructions[_current_func].emplace_back(Operation::LOADA, index.first, index.second);

            ////调用表达式子程序
            TokenType typeTest;
            auto err = analyseExpression(typeTest);
            if(err.has_value())
                return err;
            if(type != typeTest) {
                if(type == TokenType::DOUBLE && (typeTest == TokenType::INT || typeTest == TokenType::CHAR))
                    _instructions[_current_func].emplace_back(Operation::I2D);
                else if(type == TokenType::INT && typeTest == TokenType::DOUBLE)
                    _instructions[_current_func].emplace_back(Operation::D2I);
                else if(type == TokenType::CHAR) {
                    if(typeTest == TokenType::DOUBLE) {
                        _instructions[_current_func].emplace_back(Operation::D2I);
                        _instructions[_current_func].emplace_back(Operation::I2C);
                    }
                    else if(typeTest == TokenType::INT)
                        _instructions[_current_func].emplace_back(Operation::I2C);
                }

                else
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
            }

            if(type == TokenType::DOUBLE)
                _instructions[_current_func].emplace_back(Operation::DSTORE);
            else
                _instructions[_current_func].emplace_back(Operation::ISTORE);

            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
            if(next.value().GetType() == TokenType::SEMICOLON)
                return {};
            else if(next.value().GetType() == TokenType::COMMA)
                continue;
            else
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
	    }

	    return {};
	}


	// <expression> ::=
    //    <additive-expression>
    // <additive-expression> ::=
    //     <multiplicative-expression>{<additive-operator><multiplicative-expression>}
    // <additive-operator>       ::= '+' | '-'
	std::optional<CompilationError> Analyser::analyseExpression(TokenType& myType) {
	    TokenType typeTest;
        auto err = analyseMultiExpression(typeTest);
        if(err.has_value())
            return err;

        myType = typeTest;

        while(true) {
            auto next = nextToken();
            if(!next.has_value())
                return {};

            // 0是减法， 1是加法
            int mul_flag;
            if(next.value().GetType() == TokenType::PLUS_SIGN)
                mul_flag = 1;

            else if(next.value().GetType() == TokenType::MINUS_SIGN)
                mul_flag = 0;
            else {
                unreadToken();
                return {};
            }

            // 保存指令的切换栈，便于强制转换次栈顶的类型
            //// 一定要把指令迁移到当前的栈中！
            //// 一定要把栈切换回去！
            _current_func++;

            //// 以下表达式的分析产生的指令将在新的栈中保存
            err = analyseMultiExpression(typeTest);
            if(err.has_value())
                return err;

            // 类型转换
            // myType 代表当前为止计算结果的类型
            // typeTest 代表下一个项的类型
            // 如果typeTest是double而myType是int，则需要转myType为double
            // 如果typeTest是int而myType是double，则需要转typeTest为double
            // 否则不进行类型转换
            if(myType == TokenType::INT || myType == TokenType::CHAR) {
                if(typeTest == TokenType::DOUBLE) {
                    myType = TokenType ::DOUBLE;
                    //// 转换次栈顶的类型
                    _instructions[_current_func - 1].emplace_back(Operation::I2D);
                    if(mul_flag == 1)
                        _instructions[_current_func].emplace_back(Operation::DADD);
                    else
                        _instructions[_current_func].emplace_back(Operation::DSUB);
                }
                else if(typeTest == TokenType::INT || typeTest == TokenType::CHAR) {
                    myType = TokenType ::INT;
                    if(mul_flag == 1)
                        _instructions[_current_func].emplace_back(Operation::IADD);
                    else
                        _instructions[_current_func].emplace_back(Operation::ISUB);
                }
                else if(typeTest == TokenType::VOID)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
            }
            else if(myType == TokenType::DOUBLE) {
                if(typeTest == TokenType::INT || typeTest == TokenType::CHAR) {
                    myType = TokenType ::DOUBLE;
                    _instructions[_current_func].emplace_back(Operation::I2D);
                }
                // 否则不需要进行类型转换
                // 保证类型表达式只能为int或double
                if(mul_flag == 1)
                    _instructions[_current_func].emplace_back(Operation::DADD);
                else
                    _instructions[_current_func].emplace_back(Operation::DSUB);
            }
            else
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

            //// 迁移指令，清空临时栈，把栈切换回来！
            auto itr = _instructions[_current_func].begin();
            while(itr != _instructions[_current_func].end()) {
                _instructions[_current_func - 1].emplace_back(*itr);
                itr++;
            }

            _instructions[_current_func].clear();
            _instructions.erase(_current_func);
            _current_func--;
        }
        return {};
	}

	// <multiplicative-expression> ::=
    //     <cast-expression>{<multiplicative-operator><cast-expression>}
    // <multiplicative-operator> ::= '*' | '/'
    std::optional<CompilationError> Analyser::analyseMultiExpression(TokenType& myType) {
	    TokenType typeTest;
        auto err = analyseCastExpression(typeTest);
        if(err.has_value())
            return err;

        myType = typeTest;

        while(true) {
            auto next = nextToken();
            if(!next.has_value())
                return {};

            // 0是除法， 1是乘法
            int mul_flag;
            if(next.value().GetType() == TokenType::MULTIPLICATION_SIGN)
                mul_flag = 1;

            else if(next.value().GetType() == TokenType::DIVISION_SIGN)
                mul_flag = 0;
            else {
                unreadToken();
                return {};
            }

            //// 切换指令栈
            _current_func++;

            err = analyseCastExpression(typeTest);
            if(err.has_value())
                return err;

            // 类型转换
            // myType 代表当前为止计算结果的类型
            // typeTest 代表下一个项的类型
            // 如果typeTest是double而myType是int，则需要转myType为double
            // 如果typeTest是int而myType是double，则需要转typeTest为double
            // 否则不进行类型转换
            if(myType == TokenType::INT || myType == TokenType::CHAR) {
                if(typeTest == TokenType::DOUBLE) {
                    myType = TokenType ::DOUBLE;
                    ////转换次栈顶的类型
                    _instructions[_current_func-1].emplace_back(Operation::I2D);

                    if(mul_flag == 1)
                        _instructions[_current_func].emplace_back(Operation::DMUL);
                    else
                        _instructions[_current_func].emplace_back(Operation::DDIV);
                }
                else if(typeTest == TokenType::INT || typeTest == TokenType::CHAR) {
                    myType = TokenType ::INT;
                    if(mul_flag == 1)
                        _instructions[_current_func].emplace_back(Operation::IMUL);
                    else
                        _instructions[_current_func].emplace_back(Operation::IDIV);
                }
                else if(typeTest == TokenType::VOID)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
            }
            else if(myType == TokenType::DOUBLE) {
                myType = TokenType ::DOUBLE;
                if(typeTest == TokenType::INT || typeTest == TokenType::CHAR)
                    _instructions[_current_func].emplace_back(Operation::I2D);
                else if(typeTest == TokenType::VOID)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
                // 否则不需要进行类型转换
                if(mul_flag == 1)
                    _instructions[_current_func].emplace_back(Operation::DMUL);
                else
                    _instructions[_current_func].emplace_back(Operation::DDIV);
            }
            else if(myType == TokenType::VOID)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

            //// 迁移指令，清空临时栈，把栈切换回来！
            auto itr = _instructions[_current_func].begin();
            while(itr != _instructions[_current_func].end()) {
                _instructions[_current_func - 1].emplace_back(*itr);
                itr++;
            }

            _instructions[_current_func].clear();
            _instructions.erase(_current_func);
            _current_func--;
        }
        return {};
	}

	//<cast-expression> ::=
    //    {'('<type-specifier>')'}<unary-expression>
    // <type-specifier>         ::= <simple-type-specifier>
    // <simple-type-specifier>  ::= 'void'|'int'|'char'|'double'
    std::optional<CompilationError> Analyser::analyseCastExpression(TokenType& myType) {
	    std::vector<TokenType> tts;
        while(true) {
            auto next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
                unreadToken();
                break;
            }

            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);

            auto type = next.value().GetType();
            if(type == TokenType::VOID)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
            if(type != TokenType::INT && type != TokenType::CHAR && type != TokenType::DOUBLE) {
                // 则可能是基础表达式里面的 (expression)
                unreadToken();
                unreadToken();
                break;
            }

            tts.push_back(type);
            next = nextToken();
            if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        }

        TokenType typeTest;
        auto err = analyseUnaryExpression(typeTest);
        if(err.has_value())
            return err;

        if(!tts.empty())  {
            if(typeTest == TokenType::VOID)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
            auto itr = tts.rbegin();
            while(itr != tts.rend()) {
                switch (*itr){
                    case TokenType::DOUBLE :{
                        if(typeTest == TokenType::INT || typeTest == TokenType::CHAR)
                            _instructions[_current_func].emplace_back(Operation::I2D);

                        typeTest = TokenType ::DOUBLE;
                        break;
                    }

                    case TokenType ::INT :{
                        if(typeTest == TokenType::DOUBLE)
                            _instructions[_current_func].emplace_back(Operation::D2I);

                        typeTest = TokenType::INT;
                        break;
                    }

                    case TokenType ::CHAR :{
                        if(typeTest == TokenType::INT)
                            _instructions[_current_func].emplace_back(Operation::I2C);
                        else if(typeTest == TokenType::DOUBLE) {
                            _instructions[_current_func].emplace_back(Operation::D2I);
                            _instructions[_current_func].emplace_back(Operation::I2C);
                        }

                        typeTest = TokenType ::CHAR;
                        break;
                    }

                    default:
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
                }
                itr++;
            }
        }

        myType = typeTest;
        return {};
	}

    //<unary-expression> ::=
    //    [<unary-operator>]<primary-expression>
    //<unary-operator>          ::= '+' | '-'
    std::optional<CompilationError> Analyser::analyseUnaryExpression(TokenType& myType) {
	    auto next = nextToken();
	    bool flag = false;
	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);

	    if(next.value().GetType() == TokenType::PLUS_SIGN || next.value().GetType() == TokenType::MINUS_SIGN)
            flag = true;
        else
            unreadToken();

        auto tk = next.value();

        TokenType typeTest;
        auto err = analysePrimaryExpression(typeTest);
        if(err.has_value())
            return err;
        myType = typeTest;

        if(flag && tk.GetType() == TokenType::MINUS_SIGN) {
            switch (typeTest){
                case TokenType::INT: {
                    _instructions[_current_func].emplace_back(Operation::INEG);
                    break;
                }
                case TokenType ::DOUBLE: {
                    _instructions[_current_func].emplace_back(Operation::DNEG);
                    break;
                }
                case TokenType ::CHAR: {
                    _instructions[_current_func].emplace_back(Operation::INEG);
                    myType = TokenType::INT;
                    break;
                }
                default:
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
            }
        }

        return {};
	}

    // <primary-expression> ::=
    //     '('<expression>')'
    //    |<identifier>
    //    |<integer-literal>
    //    |<char-literal>
    //    |<floating-literal>
    //    |<function-call>
    std::optional<CompilationError> Analyser::analysePrimaryExpression(TokenType& myType) {
	    auto next = nextToken();
	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
	    auto tk = next.value();

	    switch(tk.GetType()) {
	        // '('<expression>')'
	        case TokenType ::LEFT_BRACKET: {
	            TokenType typeTest;
	            auto err = analyseExpression(typeTest);
	            if(err.has_value())
	                return err;
	            myType = typeTest;

	            next = nextToken();
	            if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
	            break;
	        }

	        // 在标识符下判断是变量还是函数调用
	        case TokenType ::IDENTIFIER: {
	            next = nextToken();
	            // 有左括号， 是函数调用
	            // <function-call> ::=
                //    <identifier> '(' [<expression-list>] ')'
                // <expression-list> ::=
                //    <expression>{','<expression>}
	            if(next.has_value() && next.value().GetType() == TokenType::LEFT_BRACKET) {
	                unreadToken();
	                unreadToken();
	                auto err = analyseFunctionCall(myType);
	                if(err.has_value())
	                    return err;
	            }
	            //否则，回退token，视为变量使用
	            else {
	                unreadToken();
                    if(!isUseful(tk.GetValueString()))
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
	                if(!isInitializedVariable(tk.GetValueString()))
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
                    auto index = getIndex(tk.GetValueString());
                    auto type = getType(tk.GetValueString());
                    // 加载变量地址
                    _instructions[_current_func].emplace_back(Operation::LOADA, index.first, index.second);
                    if(type == TokenType::DOUBLE)
                        _instructions[_current_func].emplace_back(Operation::DLOAD);
                    else
                        _instructions[_current_func].emplace_back(Operation::ILOAD);

                    myType = type;
	            }
                break;
	        }
	        case TokenType ::UNSIGNED_INTEGER: {
                int32_t val = std::any_cast<int32_t>(tk.GetValue());
                _instructions[_current_func].emplace_back(Operation::IPUSH, val);
                myType = TokenType ::INT;
                break;
	        }
            case TokenType ::HEXADECIMAL: {
                int32_t index = addRuntimeConsts(tk);
                _instructions[_current_func].emplace_back(Operation::LOADC, index);
                myType = TokenType ::INT;
                break;
            }
	        case TokenType::CHAR_VALUE: {
	            char val = std::any_cast<char>(tk.GetValue());
                _instructions[_current_func].emplace_back(Operation::BIPUSH, val);
                myType = TokenType ::CHAR;
                break;
	        }
	        case TokenType ::DOUBLE_VALUE: {
	            int32_t index = addRuntimeConsts(tk);
                _instructions[_current_func].emplace_back(Operation::LOADC, index);
                myType = TokenType ::DOUBLE;
                break;
	        }
            default: {
                // 意想不到的错误
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
            }
	    }
	    return {};
	}

	// <function-call> ::=
    //    <identifier> '(' [<expression-list>] ')'
    //<expression-list> ::=
    //    <expression>{','<expression>}
    std::optional<CompilationError> Analyser::analyseFunctionCall(TokenType& myType) {
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

        auto tk = next.value();

        // 分析参数列表
        // 类型不匹配的参数需要进行强制类型转换
        int32_t f_index = getFuncIndex(tk.GetValueString());

        myType = _funcs[f_index].first;

        auto &mp = _funcs[f_index].second;
        auto itr = mp.begin();

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoLeftBrace);

        while(itr != mp.end()) {
            TokenType typeTest;
            auto err = analyseExpression(typeTest);
            if(err.has_value())
                return err;

            // 判断是否需要进行隐式类型转换
            switch(*itr) {
                case TokenType ::INT:{
                    if(typeTest == TokenType::DOUBLE)
                        _instructions[_current_func].emplace_back(Operation::D2I);
                    // char到int不用转换
                    break;
                }
                case TokenType ::DOUBLE: {
                    if(typeTest == TokenType::INT || typeTest == TokenType::CHAR)
                        _instructions[_current_func].emplace_back(Operation::I2D);
                    break;
                }
                case TokenType ::CHAR: {
                    if(typeTest == TokenType::DOUBLE) {
                        _instructions[_current_func].emplace_back(Operation::D2I);
                        _instructions[_current_func].emplace_back(Operation::I2C);
                    }
                    else if(typeTest == TokenType::INT)
                        _instructions[_current_func].emplace_back(Operation::I2C);
                    break;
                }
                default:
                    unreadToken();
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamType);
            }
            itr++;

            // 判断逗号
            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);

            if(next.value().GetType() == TokenType::COMMA) {
                if(itr == mp.end())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);

                else
                    continue;
            }
            else if(next.value().GetType() == TokenType::RIGHT_BRACKET) {
                if(itr != mp.end())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);
                else {
                    unreadToken();
                    break;
                }
            }

            else
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);
        }

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
            unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoRightBrace);
        }

        _instructions[_current_func].emplace_back(Operation::CALL, f_index);
        return {};
	}

	// <return-statement> ::= 'return' [<expression>] ';'
    std::optional<CompilationError> Analyser::analyseRetStatement() {
	    ////进入此函数前已经读取return
	    auto next = nextToken();
	    int32_t _current = _nextFunc - 1;
	    if(_current < 0)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrReturnWrong);

	    const TokenType retType = _funcs[_current].first;


	    next = nextToken();
	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrReturnWrong);

	    auto type = next.value().GetType();
	    if(retType == TokenType::VOID) {
	        if(type == TokenType::SEMICOLON){
                _instructions[_current_func].emplace_back(Operation::RET);
                return {};
	        }
	        else {
	            unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
	        }
	    }

        unreadToken();

        auto err = analyseExpression(type);
        if(err.has_value())
            return err;
            //return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrReturnWrong);

        if(type == TokenType::VOID)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrReturnWrong);

	    if(retType == TokenType::DOUBLE) {
	        if(type == TokenType::INT || type == TokenType::CHAR)
	            _instructions[_current_func].emplace_back(Operation::I2D);

            _instructions[_current_func].emplace_back(Operation::DRET);
	    }
	    else if(retType == TokenType::INT || retType == TokenType::CHAR) {
	        if(type == TokenType::DOUBLE)
	            _instructions[_current_func].emplace_back(Operation::D2I);

	        if(retType == TokenType::CHAR && type != TokenType::CHAR)
                _instructions[_current_func].emplace_back(Operation::I2C);

            _instructions[_current_func].emplace_back(Operation::IRET);
	    }

	    next = nextToken();
	    if(!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
	        unreadToken();
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
	    }

	    return {};
	}

	// <assignment-expression> ::=
    //    <identifier><assignment-operator><expression>
    //// ;
    std::optional<CompilationError> Analyser::analyseAssignmentStatement() {
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

        auto tk = next.value();
        if(!isUseful(tk.GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
        if(isConstant(tk.GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);

        const auto type = getType(tk.GetValueString());
        auto index = getIndex(tk.GetValueString());
        _instructions[_current_func].emplace_back(Operation::LOADA, index.first, index.second);

        next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::ASSIGN_SIGN)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidAssignment);

        TokenType typeTest;
        auto err = analyseExpression(typeTest);
        if(err.has_value())
            return err;

        if(typeTest == TokenType::VOID)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

        if(type == TokenType::INT) {
            if(typeTest == TokenType::DOUBLE)
                _instructions[_current_func].emplace_back(Operation::D2I);
            _instructions[_current_func].emplace_back(Operation::ISTORE);
        }
        else if(type == TokenType::DOUBLE) {
            if(typeTest == TokenType::INT || typeTest == TokenType::CHAR)
                _instructions[_current_func].emplace_back(Operation::I2D);
            _instructions[_current_func].emplace_back(Operation::DSTORE);
        }
        else if(type == TokenType::CHAR) {
            if(typeTest == TokenType::DOUBLE)
                _instructions[_current_func].emplace_back(Operation::D2I);
            _instructions[_current_func].emplace_back(Operation::I2C);
            _instructions[_current_func].emplace_back(Operation::ISTORE);
        }


        if(!isInitializedVariable(tk.GetValueString())) {
            int32_t level = _current_level;
            while(level >= 0) {
                if(_uninitialized_vars[level].find(tk.GetValueString()) != _uninitialized_vars[level].end()) {
                    _vars[level][tk.GetValueString()] = _uninitialized_vars[level][tk.GetValueString()];
                    _uninitialized_vars[level].erase(tk.GetValueString());
                    break;
                }
                level --;
            }
        }

        return {};
	}



	std::optional<Token> Analyser::nextToken() {
		if (_offset == _tokens.size())
			return {};
		// 考虑到 _tokens[0..._offset-1] 已经被分析过了
		// 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
		_current_pos = _tokens[_offset].GetEndPos();
		return _tokens[_offset++];
	}

	//返回的是参数所占空间的大小，单位是slot
	int32_t Analyser::addFunc(const Token & funcName, const c0::TokenType & retType, const std::vector<TokenType> & paramTypes) {
	    if(funcName.GetType() != TokenType::IDENTIFIER)
	        return -1;

	    _funcs_index_name[_nextFunc] = funcName.GetValueString();
	    _funcs[_nextFunc] = std::make_pair(retType, paramTypes);
	    //函数表的下标、函数名在常量表中的下标、参数的slot数、层级
	    int32_t t1, t2, t3, t4;
	    t1 = _nextFunc;
	    t2 = addRuntimeConsts(funcName);
	    t3 = 0;
	    t4 = _current_level;
	    auto itr = paramTypes.begin();
	    while(itr != paramTypes.end()) {
	        if(*itr == TokenType::DOUBLE)
	            t3 += 2;
	        else
	            t3 += 1;
	        itr++;
	    }

	    std::tuple<int32_t,int32_t,int32_t,int32_t> t(t1,t2,t3,t4);
	    _runtime_funcs[_nextFunc] = t;

	    _current_func = _nextFunc;
	    _nextFunc++;
	    return t3;
	}

	bool Analyser::isFuncDeclared(const std::string & funcName) {
	    auto itr = _funcs_index_name.begin();
	    while(itr != _funcs_index_name.end()) {
	        if(itr->second == funcName)
	            return true;
	        itr++;
	    }
	    return false;
	}

	void Analyser::unreadToken() {
		if (_offset == 0)
			DieAndPrint("analyser unreads token from the begining.");
		_current_pos = _tokens[_offset - 1].GetEndPos();
		_offset--;
	}

	int32_t Analyser::addRuntimeConsts(const Token& tk) {
	    if(_runtime_consts_index.find(tk.GetValueString()) != _runtime_consts_index.end())
	        return _runtime_consts_index[tk.GetValueString()];

	    std::string s;
	    std::stringstream ss;
	    switch(tk.GetType()) {
	        case TokenType ::IDENTIFIER:
	        case TokenType ::STRING_VALUE: {
	            s = "S";
	            ss << tk.GetValueString();
	            break;
	        }
	        case TokenType ::HEXADECIMAL:
	        case TokenType::UNSIGNED_INTEGER: {
	            s = "I";
	            ss << tk.GetValueString();
	            break;
	        }
	        case TokenType ::DOUBLE_VALUE: {
	            s = "D";
	            ss << tk.GetValueString();
	            break;
	        }
            default:
                return -1;
	    }
	    std::string str = ss.str();
	    _runtime_consts[_consts_offset] = std::make_tuple(s, str);
	    _runtime_consts_index[tk.GetValueString()] = _consts_offset;
	    return _consts_offset++;
	}

	void Analyser::_add(const Token& tk, std::map<std::string, int32_t>& mp, std::map<std::string, TokenType>& mp_type, const TokenType& type) {
		if (tk.GetType() != TokenType::IDENTIFIER)
			DieAndPrint("only identifier can be added to the table.");
		mp[tk.GetValueString()] = _nextTokenIndex[_current_level];
		mp_type[tk.GetValueString()] = type;
		if(type == TokenType::DOUBLE)
            _nextTokenIndex[_current_level] += 2;
        else
            _nextTokenIndex[_current_level]++;
	}

	void Analyser::addVariable(const Token& tk, const TokenType& type) {
		_add(tk, _vars[_current_level], _vars_type[_current_level], type);
	}

	void Analyser::addConstant(const Token& tk, const TokenType& type) {
		_add(tk, _consts[_current_level], _vars_type[_current_level], type);
	}

	void Analyser::addUninitializedVariable(const Token& tk, const TokenType& type) {
		_add(tk, _uninitialized_vars[_current_level], _vars_type[_current_level], type);
	}

	std::pair<int32_t, int32_t> Analyser::getIndex(const std::string& s) {
	    for(int level = _current_level; level >= 0; level--){
	        int32_t _far = _current_level - level;
	        if(level == 0 && _far > 0)
	            _far = 1;
	        else if(level > 0)
	            _far = 0;

            if (_uninitialized_vars[level].find(s) != _uninitialized_vars[level].end())
                return std::make_pair(_far, _uninitialized_vars[level][s]);
            else if (_vars[level].find(s) != _vars[level].end())
                return std::make_pair(_far, _vars[level][s]);
            else if(_consts[level].find(s) != _consts[level].end())
                return std::make_pair(_far, _consts[level][s]);
	    }
        return std::make_pair(-1, -1);
	}

	TokenType Analyser::getType(const std::string & s) {
        int32_t _far;
        for(int level = _current_level; level >= 0; level--){
            _far = _current_level - level;

            if (_uninitialized_vars[level].find(s) != _uninitialized_vars[level].end())
                break;
            else if (_vars[level].find(s) != _vars[level].end())
                break;
            else if(_consts[level].find(s) != _consts[level].end())
                break;
        }
	    return _vars_type[_current_level - _far][s];
	}

	//仅判断在当前层级内是否被声明过
	//在声明新变量的时候调用
	bool Analyser::isDeclared(const std::string& s) {
		return _vars[_current_level].find(s) != _vars[_current_level].end()
		            || _uninitialized_vars[_current_level].find(s) != _uninitialized_vars[_current_level].end()
		            || _consts[_current_level].find(s) != _consts[_current_level].end();
	}

	//// 是否是可以使用的变量
	bool Analyser::isUseful(const std::string& s) {
        int i;
        for(i = _current_level; i >= 0; i--) {
            if(_vars[i].find(s) != _vars[i].end()
                    || _consts[i].find(s) != _consts[i].end()
                    || _uninitialized_vars[i].find(s) != _uninitialized_vars[i].end())
                return true;
        }
        return false;
	}
	bool Analyser::isInitializedVariable(const std::string&s) {
        int i;
        for(i = _current_level; i >= 0 && _vars[i].find(s) == _vars[i].end() && _consts[i].find(s) == _consts[i].end() ; i--);
        return _vars[i].find(s) != _vars[i].end() || _consts[i].find(s) != _consts[i].end();
	}

	bool Analyser::isConstant(const std::string&s) {
	    int i;
	    for(i = _current_level; i >= 0 && _consts[i].find(s) == _consts[i].end(); i--);
		return _consts[i].find(s) != _consts[i].end();
	}

	int32_t  Analyser::nextLevel(const int32_t & sp) {
        _current_level++;
	    _nextTokenIndex[_current_level] = sp;
        _vars[_current_level] = {};
        _uninitialized_vars[_current_level] = {};
        _consts[_current_level] = {};
        _vars_type[_current_level] = {};
        return _current_level;
	}

	int32_t Analyser::lastLevel() {
	    if(_current_level == 0)
	        return _current_level;
	    _vars[_current_level].clear();
	    _consts[_current_level].clear();
	    _uninitialized_vars[_current_level].clear();
        _vars_type[_current_level].clear();
	    _nextTokenIndex[_current_level]=0;
	    _current_level--;
	    return _current_level;
	}

	int32_t Analyser::getFuncIndex(const std::string& s) {
		auto itr = _funcs_index_name.begin();
		while(itr != _funcs_index_name.end()) {
			if (itr->second == s)
				return itr->first;
			itr++;
		}
		return -1;
	}
}