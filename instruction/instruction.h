#pragma once

#include <cstdint>
#include <utility>

namespace c0 {

	enum Operation {
		NOP = 0,
		BIPUSH, IPUSH,
		POP, POP2, POPN,
		DUP, DUP2,
		LOADC, LOADA,
		NEW, SNEW,
		ILOAD, DLOAD, ALOAD,
		ISTORE, DSTORE, ASTORE, 
		IASTORE, DASTORE, AASTORE,
		IADD, DADD,
		ISUB, DSUB,
		IMUL, DMUL,
		IDIV, DDIV,
		INEG, DNEG,
		ICMP, DCMP,
		I2D, D2I, I2C,
		JMP, JE, JNE, JL, JGE, JG, JLE,
		CALL,
		RET, IRET, DRET, ARET,
		IPRINT, DPRINT, CPRINT, SPRINT, PRINTL,
		ISCAN, DSCAN, CSCAN,
		ILL,
	};
	
	class Instruction final {
    public:
	    int32_t opn;
	private:
		using int32_t = std::int32_t;
	public:
		friend void swap(Instruction& lhs, Instruction& rhs);
	public:
        Instruction(Operation opr) :opn(0),_opr(opr), _x(-1), _option(-1) {}
		Instruction(Operation opr, int32_t x) : opn(0),_opr(opr), _x(x), _option(-1) {}
        Instruction(Operation opr, int32_t x, int32_t option) : opn(0),_opr(opr), _x(x), _option(option) {}
		
		Instruction() : Instruction(Operation::ILL, 0){}
		Instruction(const Instruction& i) { _opr = i._opr; _x = i._x; _option = i._option; }
		Instruction(Instruction&& i) :Instruction() { swap(*this, i); }
		Instruction& operator=(Instruction i) { swap(*this, i); return *this; }
		bool operator==(const Instruction& i) const { return _opr == i._opr && _x == i._x; }

		Operation GetOperation() const { return _opr; }
		int32_t GetX() const { return _x; }
		int32_t GetOpt() const { return _option; }

	private:
		Operation _opr;
		int32_t _x;
		int32_t _option;
	};

	inline void swap(Instruction& lhs, Instruction& rhs) {
		using std::swap;
		swap(lhs._opr, rhs._opr);
		swap(lhs._x, rhs._x);
	}
}