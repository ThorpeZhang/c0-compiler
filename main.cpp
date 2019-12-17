#include "argparse.hpp"
#include "fmt/core.h"

#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"
#include "fmts.hpp"

#include <iostream>
#include <fstream>
#include <unordered_map>

std::vector<c0::Token> _tokenize(std::istream& input) {
	c0::Tokenizer tkz(input);
	auto p = tkz.AllTokens();
	if (p.second.has_value()) {
		fmt::print(stderr, "Tokenization error: {}\n", p.second.value());
		exit(2);
	}
	return p.first;
}

const std::unordered_map<c0::Operation, std::vector<int>> paramSizeOfOperation = {
        { c0::Operation::BIPUSH, {1} },    { c0::Operation::IPUSH, {4} },
        { c0::Operation::POPN, {4} },
        { c0::Operation::LOADC, {2} },    { c0::Operation::LOADA, {2, 4} },
        { c0::Operation::SNEW, {4} },

        { c0::Operation::JMP, {2} },
        { c0::Operation::JE, {2} }, { c0::Operation::JNE, {2} }, { c0::Operation::JL, {2} }, { c0::Operation::JGE, {2} }, { c0::Operation::JG, {2} }, { c0::Operation::JLE, {2} },

        { c0::Operation::CALL, {2} },
};

void Tokenize(std::istream& input, std::ostream& output) {
	auto v = _tokenize(input);
	for (auto& it : v)
		output << fmt::format("{}\n", it);
	return;
}

void Binaryse(std::istream& input, std::ostream& output) {
    char bytes[8];
    const auto writeNBytes = [&](void* addr, int count) {
        assert(0 < count && count <= 8);
        char* p = reinterpret_cast<char*>(addr) + (count-1);
        for (int i = 0; i < count; ++i) {
            bytes[i] = *p--;
        }
        output.write(bytes, count);
    };


    //// 输入magic = 0x43303A29
    output.write("\x43\x30\x3a\x29", 4);
    //// 输入version = 0x01
    output.write("\x00\x00\x00\x01", 4);

    auto tks = _tokenize(input);
    c0::Analyser analyser(tks);
    auto p = analyser.Analyse();
    if (p.second.has_value()) {
        fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
        exit(2);
    }

    //// 输入constants_count
    auto _consts = analyser.getConst();
    uint16_t constats_count = _consts.size();
    writeNBytes(&constats_count, sizeof constats_count);

    //// 遍历常量表，输入每一行常量
    for(auto & itr : _consts) {
        auto _const = itr.second;
        std::string _type = std::get<0>(_const);
        std::string str = std::get<1>(_const);
        std::stringstream ss;
        ss << str;
        if(_type == "S") {
            output.write("\x00", 1);
            uint16_t len = str.length();
            writeNBytes(&len, sizeof len);
            output.write(str.c_str(), len);
        }
        else if(_type == "I") {
            output.write("\x01", 1);
            int32_t v;
            ss >> v;
            writeNBytes(&v, sizeof v);
        }
        else if(_type == "D") {
            output.write("\x02", 1);
            double v;
            ss >> v;
            writeNBytes(&v, sizeof v);
        }
        else
            assert(("unexpected error", false));
    }

    auto to_binary = [&](const std::vector<c0::Instruction>& v) {
        uint16_t instructions_count = v.size();
        writeNBytes(&instructions_count, sizeof instructions_count);
        for (auto& ins : v) {
            uint8_t op = ins.getBinaryInstruction();
            printf("%02x\n",op);
            writeNBytes(&op, sizeof op);
            if (auto it = paramSizeOfOperation.find(ins.GetOperation()); it != paramSizeOfOperation.end()) {
                auto paramSizes = it->second;
                switch (paramSizes[0]) {
                    case 1: {
                        uint8_t x = ins.GetX();
                        writeNBytes(&x, 1);
                        break;
                    }
                    case 2: {
                        uint16_t x = ins.GetX();
                        writeNBytes(&x, 2);
                        break;
                    }
                    case 4: {
                        uint32_t x = ins.GetX();
                        writeNBytes(&x, 4);
                        break;
                    }
                    default:
                        assert(("unexpected error", false));
                }
                if (paramSizes.size() == 2) {
                    switch (paramSizes[1]) {
                        case 1: {
                            uint8_t y = ins.GetOpt();
                            writeNBytes(&y, 1);
                            break;
                        }
                        case 2: {
                            uint16_t y = ins.GetOpt();
                            writeNBytes(&y, 2);
                            break;
                        }
                        case 4: {
                            uint32_t y = ins.GetOpt();
                            writeNBytes(&y, 4);
                            break;
                        }
                        default:
                            assert(("unexpected error", false));
                    }
                }
            }
        }
    };

    //// 开始输出start代码
    auto _start = analyser.getStartCode();
    to_binary(_start);

    //// 输出函数表
    auto _funcs = analyser.getFuncs();
    uint16_t  functions_count = _funcs.size();
    writeNBytes(&functions_count, sizeof functions_count);
    //// 遍历函数表，输出函数指令序列
    auto functions = p.first;
    for(auto & fun : _funcs) {
        auto tup = fun.second;
        uint16_t v;
        v = std::get<1>(tup);   writeNBytes(&v, sizeof v);
        //std::cout<<v<<std::endl;
        v = std::get<2>(tup);   writeNBytes(&v, sizeof v);
        //std::cout<<v<<std::endl;
        v = std::get<3>(tup);   writeNBytes(&v, sizeof v);
        //std::cout<<v<<std::endl;

        v = std::get<0>(tup);
        //std::cout<<v<<std::endl;
        to_binary(functions[v]);
    }
}

void Analyse(std::istream& input, std::ostream& output){
	auto tks = _tokenize(input);
	c0::Analyser analyser(tks);
	auto p = analyser.Analyse();
	if (p.second.has_value()) {
		fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
		exit(2);
	}

	//// 输出汇编指令
	//// 输出常量表
    output << fmt::format(".constants:\n");
	auto _const = analyser.getConst();
	for(auto & itr : _const) {
	    std::string str = std::get<1>(itr.second);
        std::string type = std::get<0>(itr.second);
        if(type == "S") {
            std::stringstream ss;
            ss << '\"'; ss << str;  ss << '\"';
            str.clear();
            str = ss.str();
        }
        output << fmt::format("{} {} {}\n", itr.first, type, str);
	}

	//// 输出开始指令
    output << fmt::format(".start:\n");
    auto _start = analyser.getStartCode();
    int32_t _i = 0;
    for(auto & itr : _start)
        output << fmt::format("{}\t{}\n", _i++, itr);

    //// 输出函数表
    output << fmt::format(".functions:\n");
    auto _func = analyser.getFuncs();
    for(auto & itr : _func) {
        auto& t = itr.second;
        output << fmt::format("{} {} {} {}\n", std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t));
    }

    //// 输出各函数代码
	auto v = p.first;
	for (auto& it : v) {
        output << fmt::format(".F{}:\n", it.first);
        _i = 0;
        for(auto & itr : it.second)
            output << fmt::format("{}\t{}\n", _i++, itr);
	}
	return;
}

int main(int argc, char** argv) {
	argparse::ArgumentParser program("cc0");
    program.add_argument("input")
            .required()
            .help("speicify the file to be assembled/executed.");
    program.add_argument("-s")
            .default_value(false)
            .implicit_value(true)
            .help("compile the text input file.");
    program.add_argument("-c")
            .default_value(false)
            .implicit_value(true)
            .help("assemble the text input file.");
    program.add_argument("-o", "--output")
            .required()
            .default_value(std::string("-"))
            .help("specify the output file.");

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		fmt::print(stderr, "{}\n\n", err.what());
		program.print_help();
		exit(2);
	}

	auto input_file = program.get<std::string>("input");
	auto output_file = program.get<std::string>("--output");
	std::istream* input;
	std::ostream* output;
	std::ifstream inf;
	std::ofstream outf;
	if (input_file != "-") {
		inf.open(input_file, std::ios::in);
		if (!inf) {
			fmt::print(stderr, "Fail to open {} for reading.\n", input_file);
			exit(2);
		}
		input = &inf;
	}
	else
		input = &std::cin;
	if (output_file != "-") {
		outf.open(output_file, std::ios::out | std::ios::trunc);
		if (!outf) {
			fmt::print(stderr, "Fail to open {} for writing.\n", output_file);
			exit(2);
		}
		output = &outf;
	}
	else
		output = &std::cout;
	if (program["-s"] == true && program["-c"] == true) {
		fmt::print(stderr, "You can only perform compile or assemble at one time.");
		exit(2);
	}
	if (program["-s"] == true) {
        Analyse(*input, *output);
	}
	else if (program["-c"] == true) {

		Binaryse(*input, *output);
	}
	else {
		fmt::print(stderr, "You must choose tokenization or syntactic analysis.");
		exit(2);
	}
	return 0;
}