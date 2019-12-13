#include "analyser.h"

#include <climits>

namespace c0 {
	std::pair<std::vector<std::vector<Instruction>>, std::optional<CompilationError>> Analyser::Analyse() {
		auto err = analyseC0Program();
		if (err.has_value())
			return std::make_pair(std::vector<std::vector<Instruction>>(), err);
		else
			return std::make_pair(_instructions, std::optional<CompilationError>());
	}

	// <C0-program> ::=
    //    {<variable-declaration>}{<function-definition>}
    std::optional<CompilationError> Analyser::analyseC0Program() {
	    auto err = analyseDeclaration();
	    if(err.has_value())
	        return err;

	    err = analyseFunctionDefinition();
	    if(err.has_value())
	        return err;

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
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            if(next.value().GetType() != TokenType::IDENTIFIER) {
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
            if(next.value().GetType() == TokenType::EQUAL_SIGN) {
                addVariable(identifier, type);
                //// 需要在此处调用表达式分析子程序
                TokenType typeTest;
                auto err = analyseExpression(typeTest);
                if(err.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
                if(typeTest != type)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);

                next = nextToken();
                if(!next.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);

                if(next.value().GetType() == TokenType::COMMA)
                    continue;
                else if(next.value().GetType() == TokenType::SEMICOLON)
                    break;
                else {
                    unreadToken();
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
                }

            }
            else if(next.value().GetType() == TokenType::COMMA || next.value().GetType() == TokenType::SEMICOLON) {
                addUninitializedVariable(identifier, type);
                if(type == TokenType::INT || type == TokenType::CHAR)
                    _instructions[_current_func].emplace_back(Operation::SNEW, 1);
                else if(type == TokenType::DOUBLE)
                    _instructions[_current_func].emplace_back(Operation::SNEW, 1);

                if(next.value().GetType() == TokenType::SEMICOLON)
                    break;
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

	    next = nextToken();
	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);

	    auto type = next.value().GetType();
	    if(type != TokenType::INT && type != TokenType::DOUBLE && type != TokenType::CHAR) {
	        unreadToken();
	        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableType);
	    }

	    while(true) {
            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            if(next.value().GetType() != TokenType::IDENTIFIER) {
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            }

            addConstant(next.value(), type);

            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
            if(next.value().GetType() != TokenType::EQUAL_SIGN) {
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
            }

            ////调用表达式子程序
            TokenType typeTest;
            auto err = analyseExpression(typeTest);
            if(err.has_value())
                return err;
            if(type != typeTest)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);

            next = nextToken();
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
            if(next.value().GetType() == TokenType::SEMICOLON)
                break;
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
            if(myType == TokenType::INT) {
                if(typeTest == TokenType::DOUBLE) {
                    myType = TokenType ::DOUBLE;
                    //// 转换次栈顶的类型
                    _instructions[_current_func - 1].emplace_back(Operation::IADD);
                    if(mul_flag == 1)
                        _instructions[_current_func].emplace_back(Operation::DADD);
                    else
                        _instructions[_current_func].emplace_back(Operation::DSUB);
                }
                else if(typeTest == TokenType::INT) {
                    myType = TokenType ::INT;
                    if(mul_flag == 1)
                        _instructions[_current_func].emplace_back(Operation::IADD);
                    else
                        _instructions[_current_func].emplace_back(Operation::ISUB);
                }
            }
            else if(myType == TokenType::DOUBLE) {
                if(typeTest == TokenType::INT) {
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

            err = analyseCastExpression(typeTest);
            if(err.has_value())
                return err;

            // 类型转换
            // myType 代表当前为止计算结果的类型
            // typeTest 代表下一个项的类型
            // 如果typeTest是double而myType是int，则需要转myType为double
            // 如果typeTest是int而myType是double，则需要转typeTest为double
            // 否则不进行类型转换
            if(myType == TokenType::INT) {
                if(typeTest == TokenType::DOUBLE) {
                    myType = TokenType ::DOUBLE;
                    ////如何转换次栈顶的类型？
                }
                else if(typeTest == TokenType::INT) {
                    myType = TokenType ::INT;
                    if(mul_flag == 1)
                        _instructions[_current_func].emplace_back(Operation::IMUL);
                    else
                        _instructions[_current_func].emplace_back(Operation::IDIV);
                }
            }
            else if(myType == TokenType::DOUBLE) {
                if(typeTest == TokenType::INT) {
                    myType = TokenType ::DOUBLE;
                    _instructions[_current_func].emplace_back(Operation::I2D);
                }
                // 否则不需要进行类型转换
                // 保证类型表达式只能为int或double
                if(mul_flag == 1)
                    _instructions[_current_func].emplace_back(Operation::DMUL);
                else
                    _instructions[_current_func].emplace_back(Operation::DDIV);
            }
            else
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidType);
        }
	}

	//<cast-expression> ::=
    //    {'('<type-specifier>')'}<unary-expression>
    // <type-specifier>         ::= <simple-type-specifier>
    // <simple-type-specifier>  ::= 'void'|'int'|'char'|'double'
    std::optional<CompilationError> Analyser::analyseCastExpression(TokenType& myType) {
	    std::vector<TokenType> tts;
        while(true) {
            auto next = nextToken();
            if(!next.has_value())
                break;
            if(next.value().GetType() != TokenType::LEFT_BRACKET) {
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
            if(!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
            if(next.value().GetType() != TokenType::RIGHT_BRACKET)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        }

        TokenType typeTest;
        auto err = analyseUnaryExpression(typeTest);
        if(err.has_value())
            return err;

        if(!tts.empty())  {

            auto itr = tts.rbegin();
            while(itr != tts.rend()) {
                switch (*itr){
                    case TokenType::DOUBLE :{
                        if(typeTest == TokenType::INT)
                            _instructions[_current_func].emplace_back(Operation::I2D);

                        typeTest = TokenType ::DOUBLE;
                        break;
                    }

                    case TokenType ::INT :{
                        if(typeTest == TokenType::DOUBLE)
                            _instructions[_current_func].emplace_back(Operation::D2I);

                        typeTest = TokenType::INT;
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
	    std::any tk;
	    if(!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);

	    if(next.value().GetType() == TokenType::PLUS_SIGN || next.value().GetType() == TokenType::MINUS_SIGN) {
	        flag = true;
            tk = next.value();
	    }
        else
            unreadToken();

        TokenType typeTest;
        auto err = analysePrimaryExpression(typeTest);
        myType = typeTest;
        if(err.has_value())
            return err;

        if(flag && std::any_cast<Token>(tk).GetType() == TokenType::MINUS_SIGN) {
            switch (typeTest){
                case TokenType::INT: {
                    _instructions[_current_func].emplace_back(Operation::INEG);
                    break;
                }
                case TokenType ::DOUBLE: {
                    _instructions[_current_func].emplace_back(Operation::INEG);
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
	        case TokenType ::LEFT_BRACKET: {
	            TokenType typeTest;
	            auto err = analyseExpression(typeTest);
	            if(err.has_value())
	                return err;
	            myType = typeTest;
	            next = nextToken();
	            if(!next.has_value())
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
	                // 分析参数列表
	                // 类型不匹配的参数需要进行强制类型转换
	                int32_t f_index = getFuncIndex(tk.GetValueString());

                    if(_funcs[f_index].first != TokenType::VOID)
                        return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
	                if(_funcs[f_index].first != TokenType::CHAR)
	                    myType = _funcs[f_index].first;
	                else
                        myType = TokenType ::INT;

	                auto &mp = _funcs[f_index].second;
	                auto itr = mp.begin();

	                while(itr != mp.end()) {
	                    TokenType typeTest;
	                    analyseExpression(typeTest);

	                    // 判断是否需要进行隐式类型转换
	                    if(*itr != typeTest) {
	                        if(*itr == TokenType::INT && typeTest == TokenType::DOUBLE)
                                _instructions[_current_func].emplace_back(Operation::D2I);
	                        else if(*itr == TokenType::DOUBLE && typeTest == TokenType::INT)
                                _instructions[_current_func].emplace_back(Operation::I2D);
                            else
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
	                        if(itr == mp.end())
	                            break;
                            else
                                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);
	                    }

	                    else
                            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidFunctionParamCount);
	                }
                    _instructions[_current_func].emplace_back(Operation::CALL, f_index);
	            }
	            //否则，回退token，视为变量使用
	            else {
	                unreadToken();
                    auto index = getIndex(tk.GetValueString());
                    auto type = getType(tk.GetValueString());
                    _instructions[_current_func].emplace_back(Operation::LOADA, index.first, index.second);
                    if(type == TokenType::DOUBLE) {
                        _instructions[_current_func].emplace_back(Operation::DLOAD);
                        myType = TokenType ::DOUBLE;
                    }
                    else {
                        _instructions[_current_func].emplace_back(Operation::ILOAD);
                        myType = TokenType ::INT;
                    }
	            }
                break;
	        }
	        case TokenType ::UNSIGNED_INTEGER: {
                int32_t val = std::any_cast<int32_t>(tk.GetValue());
                _instructions[_current_func].emplace_back(Operation::IPUSH, val);
                myType = TokenType ::INT;
                break;
	        }
	        case TokenType::CHAR_VALUE: {
	            char val = std::any_cast<char>(tk.GetValue());
                _instructions[_current_func].emplace_back(Operation::BIPUSH, val);
                myType = TokenType ::INT;
                break;
	        }
	        case TokenType ::DOUBLE_VALUE: {
	            double val = std::any_cast<double>(tk.GetValue());
	            int32_t index = addRuntimeConsts(tk);
                _instructions[_current_func].emplace_back(Operation::LOADC, index);
                myType = TokenType ::DOUBLE;
                break;
	        }
            default: {
                unreadToken();
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
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

	    _runtime_funcs[_nextFunc] = std::make_tuple(t1, t2, t3, t4);
	    return t3;
	}

	bool Analyser::isFuncDeclared(const std::string & funcName) {
	    auto itr = _funcs_index_name.begin();
	    while(itr != _funcs_index_name.end()) {
	        if(*itr == funcName)
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

	int32_t Analyser::addRuntimeConsts(const c0::Token & tk) {
	    _runtime_consts[_consts_offset]=tk;
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
		_add(tk, _consts[_current_level], _consts_type[_current_level], type);
	}

	void Analyser::addUninitializedVariable(const Token& tk, const TokenType& type) {
		_add(tk, _uninitialized_vars[_current_level], _consts_type[_current_level], type);
	}

	std::pair<int32_t, int32_t> Analyser::getIndex(const std::string& s) {
	    for(int level = _current_level; level >= 0; level--){
            if (_uninitialized_vars[level].find(s) != _uninitialized_vars[level].end())
                return std::make_pair(_current_level - level, _uninitialized_vars[level][s]);
            else if (_vars[level].find(s) != _vars[level].end())
                return std::make_pair(_current_level - level, _vars[level][s]);
            else if(_consts[level].find(s) != _consts[level].end())
                return std::make_pair(_current_level - level, _consts[level][s]);
	    }
	}

	TokenType Analyser::getType(const std::string & s) {
	    auto index = getIndex(s);
	    int32_t i = index.first;
	    if(_uninitialized_vars_type[i].find(s) != _uninitialized_vars_type[i].end())
	        return _uninitialized_vars_type[i][s];
	    else if(_vars_type[i].find(s) != _vars_type[i].end())
	        return _vars_type[i][s];
        else
            return _consts_type[i][s];
	}

	//仅判断在当前层级内是否被声明过
	//在声明新变量的时候调用
	bool Analyser::isDeclared(const std::string& s) {
		return _vars[_current_level].find(s) != _vars[_current_level].end()
		            || _uninitialized_vars[_current_level].find(s) != _uninitialized_vars[_current_level].end()
		            || _consts[_current_level].find(s) != _consts[_current_level].end();
	}

	bool Analyser::isUninitializedVariable(const std::string& s) {
        int i;
        for(i = _current_level; i >= 0 && _uninitialized_vars[i].find(s) == _uninitialized_vars[i].end(); i--);
        return _uninitialized_vars[i].find(s) != _uninitialized_vars[i].end();
	}
	bool Analyser::isInitializedVariable(const std::string&s) {
        int i;
        for(i = _current_level; i >= 0 && _vars[i].find(s) == _vars[i].end(); i--);
        return _vars[i].find(s) != _vars[i].end();
	}

	bool Analyser::isConstant(const std::string&s) {
	    int i;
	    for(i = _current_level; i >= 0 && _consts[i].find(s) == _consts[i].end(); i--);
		return _consts[i].find(s) != _consts[i].end();
	}

	int32_t  Analyser::nextLevel(const int32_t& sp) {
	    _current_level++;
	    _nextTokenIndex[_current_level] = sp;
	    return _current_level;
	}

	int32_t Analyser::lastLevel() {
	    if(_current_level == 0)
	        return _current_level;
	    _vars[_current_level].clear();
	    _consts[_current_level].clear();
	    _uninitialized_vars[_current_level].clear();
        _vars_type[_current_level].clear();
        _consts_type[_current_level].clear();
        _uninitialized_vars_type[_current_level].clear();
	    _nextTokenIndex[_current_level]=0;
	    _current_level--;
	    return _current_level;
	}

	int32_t Analyser::getFuncIndex(const std::string& s) {
		auto itr = _funcs_index_name.begin();
		while(itr != _funcs_index_name.end()) {
			if (*itr == s)
				return itr - _funcs_index_name.begin();
			itr++;
		}
		return -1;
	}
}