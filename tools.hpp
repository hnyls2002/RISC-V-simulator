//
// Created by m1375 on 2022/6/22.
//

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <iostream>

namespace hnyls2002 {
    enum InsType {
        LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU, LB, LH, LW, LBU, LHU, SB, SH, SW, ADDI,
        SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
    };
    const std::string InsStr[] = {
            "LUI", "AUIPC", "JAL", "JALR", "BEQ", "BNE", "BLT", "BGE", "BLTU",
            "BGEU", "LB", "LH", "LW", "LBU", "LHU", "SB", "SH", "SW", "ADDI",
            "SLTI", "SLTIU", "XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI", "ADD",
            "SUB", "SLL", "SLT", "SLTU", "XOR", "SRL", "SRA", "OR", "AND"
    };

    enum CodeType {
        R_type, I_type, S_type, B_type, U_type, J_type
    };

    struct Ins {
        u_int32_t opcode{}, rd{}, rs1{}, rs2{}, fun3{}, fun7{}, imm{}, shamt{};// shift amount
        InsType ins_type{};
    };

    uint32_t Pick(uint32_t x, uint32_t r, uint32_t l) {// [r:l] -> [r-l:0]
        return x >> l & ((1 << (r - l + 1)) - 1);
    }

    u_int32_t SExt(uint32_t x, uint32_t h) { // using the highest bit to extend
        if (1 << h & x)x = x | (-(1 << h));
        return x;
    }

}

#endif