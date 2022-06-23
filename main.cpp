#include <iostream>
#include <bitset>
#include "decode.hpp"

#define DIR "testcases_for_riscv/"

using namespace hnyls2002;

uint32_t memory[500000], reg[32], pc;

uint32_t HexStrToInt(const std::string &str) {
    uint32_t ret = 0;
    for (auto ch: str)
        ret = (ret << 4) + (ch <= '9' ? ch - '0' : ch - 'A' + 10);
    return ret;
}

void Init() {
    std::string info;
    uint32_t addr = 0, offset;
    while (std::cin >> info) {
        if (info[0] == '@')addr = HexStrToInt(info.substr(1, 8)), offset = 0;
        else memory[addr + offset++] = HexStrToInt(info);
    }
}

uint32_t ReadIns() {
    uint32_t ret = 0;
    for (int i = 0; i < 4; ++i)
        ret = ret + (memory[pc++] << (i * 8));
    return ret;
}

uint32_t ReadMem(int p, int k) {// k range in [3,0]
    uint32_t ret = 0;
    for (int i = p + k; i >= p; --i)
        ret = (ret << 8) + memory[i];
    return ret;
}

void WriteMem(uint32_t x, int p, int k) {// write and read are both little-endian
    for (int i = p; i <= p + k; ++i)
        memory[i] = x & 255, x >>= 8;
}

int main() {
    Init();
    while (true) {
        auto code = ReadIns();
        if (code == 0x0ff00513) {
            std::cout << std::dec << (reg[10] & ((uint32_t) 255)) << std::endl;
            break;
        }
        auto ins = Decode(code);
        switch (ins.ins_type) {
            case LB:
                reg[ins.rd] = SExt(ReadMem(reg[ins.rs1] + ins.imm, 0), 7);
                break;
            case LH:
                reg[ins.rd] = SExt(ReadMem(reg[ins.rs1] + ins.imm, 1), 15);
                break;
            case LW:
                reg[ins.rd] = ReadMem(reg[ins.rs1] + ins.imm, 3);
                break;
            case LBU:
                reg[ins.rd] = ReadMem(reg[ins.rs1] + ins.imm, 0);
                break;
            case LHU:
                reg[ins.rd] = ReadMem(reg[ins.rs1] + ins.imm, 1);
                break;
            case SB:
                WriteMem(reg[ins.rs2], reg[ins.rs1] + ins.imm, 0);
                break;
            case SH:
                WriteMem(reg[ins.rs2], reg[ins.rs1] + ins.imm, 1);
                break;
            case SW:
                WriteMem(reg[ins.rs2], reg[ins.rs1] + ins.imm, 3);
                break;
            case ADD:
                reg[ins.rd] = reg[ins.rs1] + reg[ins.rs2];
                break;
            case ADDI:
                reg[ins.rd] = reg[ins.rs1] + ins.imm;
                break;
            case SUB:
                reg[ins.rd] = reg[ins.rs1] - reg[ins.rs2];
                break;
            case LUI:
                reg[ins.rd] = ins.imm;
                break;
            case AUIPC:
                reg[ins.rd] = pc - 4 + ins.imm;
                break;
            case XOR:
                reg[ins.rd] = reg[ins.rs1] ^ reg[ins.rs2];
                break;
            case XORI:
                reg[ins.rd] = reg[ins.rs1] ^ ins.imm;
                break;
            case OR:
                reg[ins.rd] = reg[ins.rs1] | reg[ins.rs2];
                break;
            case ORI:
                reg[ins.rd] = reg[ins.rs1] | ins.imm;
                break;
            case AND:
                reg[ins.rd] = reg[ins.rs1] & reg[ins.rs2];
                break;
            case ANDI:
                reg[ins.rd] = reg[ins.rs1] & ins.imm;
                break;
            case SLL:
                reg[ins.rd] = reg[ins.rs1] << (reg[ins.rs2] & 31);
                break;
            case SLLI:
                reg[ins.rd] = reg[ins.rs1] << (ins.shamt);
                break;
            case SRL:
                reg[ins.rd] = reg[ins.rs1] >> (reg[ins.rs2] & 31);
                break;
            case SRLI:
                reg[ins.rd] = reg[ins.rs1] >> (ins.shamt);
                break;
            case SRA:
                reg[ins.rd] = ((int32_t) reg[ins.rs1]) >> (ins.rs2 & 31);
                break;
            case SRAI:
                reg[ins.rd] = ((int32_t) reg[ins.rs1]) >> ins.shamt;
                break;
            case SLT:// signed less than
                reg[ins.rd] = ((int32_t) reg[ins.rs1]) < ((int32_t) reg[ins.rs2]) ? 1 : 0;
                break;
            case SLTI:
                reg[ins.rd] = ((int32_t) reg[ins.rs1]) < ((int32_t) ins.imm) ? 1 : 0;
                break;
            case SLTU:
                reg[ins.rd] = reg[ins.rs1] < reg[ins.rs2] ? 1 : 0;
                break;
            case SLTIU:
                reg[ins.rd] = reg[ins.rs1] < ins.imm ? 1 : 0;
                break;
            case BEQ:
                if (reg[ins.rs1] == reg[ins.rs2])pc += ins.imm - 4;
                break;
            case BNE:
                if (reg[ins.rs1] != reg[ins.rs2])pc += ins.imm - 4;
                break;
            case BLT:
                if (((int32_t) reg[ins.rs1]) < ((int32_t) reg[ins.rs2]))pc += ins.imm - 4;
                break;
            case BGE:
                if (((int32_t) reg[ins.rs1]) >= ((int32_t) reg[ins.rs2]))pc += ins.imm - 4;
                break;
            case BLTU:
                if (reg[ins.rs1] < reg[ins.rs2])pc += ins.imm - 4;
                break;
            case BGEU:
                if (reg[ins.rs1] >= reg[ins.rs2])pc += ins.imm - 4;
                break;
            case JAL:
                reg[ins.rd] = pc;
                pc += ins.imm - 4;
                break;
            case JALR:
                uint32_t tmp = pc;
                pc = (reg[ins.rs1] + ins.imm) & ~1;
                reg[ins.rd] = tmp;
                break;
        }
        reg[0] = 0;
/*
        printf("inst type: %s, rs1: %d, rs2: %d, imm: %d, rd:%d\n", InsStr[ins.ins_type].c_str(), ins.rs1,
               ins.rs2, ins.imm, ins.rd);
        printf("reg[0] = %d\n", reg[0]);
        printf("reg[11] = %d\n", reg[11]);
        for (int i = 0; i < 32; ++i)
            printf("%u\n", reg[i]);
        printf("pc = %u\n", pc);
*/
    }
    return 0;
}
