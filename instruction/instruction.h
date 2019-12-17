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

		void set_X(const int32_t& x){_x = x;}

        uint8_t getBinaryInstruction() const{
            switch (_opr) {
                case c0::NOP:
                    return 0x00;
                case c0::BIPUSH:
                    return 0x01;
                case c0::IPUSH:
                    return 0x02;
                case c0::POP:
                    return 0x04;
                case c0::POP2:
                    return 0x05;
                case c0::POPN:
                    return 0x06;
                case c0::DUP:
                    return 0x07;
                case c0::DUP2:
                    return 0x08;
                case c0::LOADC:
                    return 0x09;
                case c0::LOADA:
                    return 0x0a;
                case c0::NEW:
                    return 0x0b;
                case c0::SNEW:
                    return 0x0c;
                case c0::ILOAD:
                    return 0x10;
                case c0::DLOAD:
                    return 0x11;
                case c0::ALOAD:
                    return 0x12;
                case c0::ISTORE:
                    return 0x20;
                case c0::DSTORE:
                    return 0x21;
                case c0::ASTORE:
                    return 0x22;
                case c0::IASTORE:
                    return 0x28;
                case c0::DASTORE:
                    return 0x29;
                case c0::AASTORE:
                    return 0x2a;
                case c0::IADD:
                    return 0x30;
                case c0::DADD:
                    return 0x31;
                case c0::ISUB:
                    return 0x34;
                case c0::DSUB:
                    return 0x35;
                case c0::IMUL:
                    return 0x38;
                case c0::DMUL:
                    return 0x39;
                case c0::IDIV:
                    return 0x3c;
                case c0::DDIV:
                    return 0x3d;
                case c0::INEG:
                    return 0x40;
                case c0::DNEG:
                    return 0x41;
                case c0::ICMP:
                    return 0x44;
                case c0::DCMP:
                    return 0x45;
                case c0::I2D:
                    return 0x60;
                case c0::D2I:
                    return 0x61;
                case c0::I2C:
                    return 0x62;
                case c0::JMP:
                    return 0x70;
                case c0::JE:
                    return 0x71;
                case c0::JNE:
                    return 0x72;
                case c0::JL:
                    return 0x73;
                case c0::JGE:
                    return 0x74;
                case c0::JG:
                    return 0x75;
                case c0::JLE:
                    return 0x76;
                case c0::CALL:
                    return 0x80;
                case c0::RET:
                    return 0x88;
                case c0::IRET:
                    return 0x89;
                case c0::DRET:
                    return 0x8a;
                case c0::ARET:
                    return 0x8b;
                case c0::IPRINT:
                    return 0xa0;
                case c0::DPRINT:
                    return 0xa1;
                case c0::CPRINT:
                    return 0xa2;
                case c0::SPRINT:
                    return 0xa3;
                case c0::PRINTL:
                    return 0xaf;
                case c0::ISCAN:
                    return 0xb0;
                case c0::DSCAN:
                    return 0xb1;
                case c0::CSCAN:
                    return 0xb2;
                default:
                    return 0x00;
            }
        }

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