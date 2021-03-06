#ifndef EXPRESSER_INSTRUCTION_H
#define EXPRESSER_INSTRUCTION_H

#include <cstring>
#include <utility>

#include "Types.h"

namespace expresser {
    enum Operation : uint8_t {
        NOP = 0x00,
        BIPUSH = 0x01,
        IPUSH = 0x02,
        POP = 0x03,
        POP2 = 0x04,
        POPN = 0x05,
        DUP = 0x06,
        DUP2 = 0x08,
        LOADC = 0x09,
        LOADA = 0x0a,
        NEW = 0x0B,
        SNEW = 0x0C,
        ILOAD = 0x10,
        DLOAD = 0x11,
        ALOAD = 0x12,
        IALOAD = 0x18,
        DALOAD = 0x19,
        AALOAD = 0x1A,
        ISTORE = 0x20,
        DSTORE = 0x21,
        ASTORE = 0x22,
        IASTORE = 0x28,
        DASTORE = 0x29,
        AASTORE = 0x2a,
        IADD = 0x30,
        DADD = 0x31,
        ISUB = 0x34,
        DSUB = 0x35,
        IMUL = 0x38,
        DMUL = 0x39,
        IDIV = 0x3c,
        DDIV = 0x3d,
        INEG = 0x40,
        DNEG = 0x41,
        ICMP = 0x44,
        DCMP = 0x45,
        I2D = 0x60,
        D2I = 0x61,
        I2C = 0x62,
        JMP = 0x70,
        JE = 0x71,
        JNE = 0x72,
        JL = 0x73,
        JGE = 0x74,
        JG = 0x75,
        JLE = 0x76,
        CALL = 0x80,
        RET = 0x88,
        IRET = 0x89,
        DRET = 0x8a,
        ARET = 0x8b,
        IPRINT = 0xa0,
        DPRINT = 0xa1,
        CPRINT = 0xa2,
        SPRINT = 0xa3,
        PRINTL = 0xaf,
        ISCAN = 0xb0,
        DSCAN = 0xb1,
        CSCAN = 0xb2
    };

    // 取反，跳转到else块
    const static std::map<TokenType, Operation> if_jmp_map{
            {LESS,         JGE},
            {LESSEQUAL,    JG},
            {GREATER,      JLE},
            {GREATEREQUAL, JL},
            {EQUAL,        JNE},
            {NOTEQUAL,     JE}
    };
    const static std::map<Operation, Operation> reverse_map{
            {JL,  JGE},
            {JLE, JG},
            {JG,  JLE},
            {JGE, JL},
            {JE,  JNE},
            {JNE, JE},
    };

    struct InstructionParam {
        uint8_t _size{};
        uint8_t _value[4] = {0};
    };

    class Instruction {
    private:
        uint32_t _index{};
        Operation _opcode{};
        std::pair<InstructionParam, InstructionParam> _params;

    public:
        Instruction() = default;

        Instruction(uint32_t index, Operation _op) {
            _index = index;
            _opcode = _op;
            _params.first._size = 0;
            _params.second._size = 0;
        }

        Instruction(uint32_t index, Operation _op, uint8_t _size1, int32_t param1) {
            _index = index;
            _opcode = _op;
            _params.first._size = _size1;
            ::memcpy(_params.first._value, &param1, _size1);
            _params.second._size = 0;
        }

        Instruction(uint32_t index, Operation _op, uint8_t _size1, int32_t param1, uint8_t _size2, int32_t param2) {
            _index = index;
            _opcode = _op;
            _params.first._size = _size1;
            ::memcpy(_params.first._value, &param1, _size1);
            _params.second._size = _size2;
            ::memcpy(_params.second._value, &param2, _size2);
        }

        void SetIndex(uint32_t index) {
            _index = index;
        }

        uint32_t GetIndex() {
            return _index;
        }

        Operation GetOperation() {
            return _opcode;
        }

        std::pair<InstructionParam, InstructionParam> GetParams() {
            return _params;
        }

        std::vector<uint8_t> ToBinary() {
            std::vector<uint8_t> result;
            auto opcode = (uint8_t) _opcode;
            result.push_back(opcode);
            if (_params.first._size != 0)
                for (int i = _params.first._size - 1; i >= 0; i--)
                    result.push_back(_params.first._value[i]);
            if (_params.second._size != 0)
                for (int i = _params.second._size - 1; i >= 0; i--)
                    result.push_back(_params.second._value[i]);
            return result;
        }
    };
}

#endif //EXPRESSER_INSTRUCTION_H
