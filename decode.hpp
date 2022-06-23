//
// Created by m1375 on 2022/6/22.
//

#ifndef DECODE_HPP
#define DECODE_HPP

#include <map>
#include <tuple>
#include "tools.hpp"

namespace hnyls2002 {
    std::map<u_int32_t, CodeType> code_map = {
            {0b0110111, U_type},
            {0b0010111, U_type},
            {0b1101111, J_type},
            {0b1100111, I_type},
            {0b1100011, B_type},
            {0b0000011, I_type},
            {0b0100011, S_type},
            {0b0010011, I_type},
            {0b0110011, R_type}
    };

    typedef std::tuple<u_int32_t, u_int32_t, u_int32_t> Triple;

    std::map<Triple, InsType> ins_map = {
            //U-type
            {Triple{0b0110111, 0, 0},             LUI},
            {Triple{0b0010111, 0, 0},             AUIPC},
            //J-type
            {Triple{0b1101111, 0, 0,},            JAL},
            //I-type
            {Triple{0b1100111, 0b000, 0},         JALR},
            //B-type
            {Triple{0b1100011, 0b000, 0},         BEQ},
            {Triple{0b1100011, 0b001, 0},         BNE},
            {Triple{0b1100011, 0b100, 0},         BLT},
            {Triple{0b1100011, 0b101, 0},         BGE},
            {Triple{0b1100011, 0b110, 0},         BLTU},
            {Triple{0b1100011, 0b111, 0},         BGEU},
            //I-type
            {Triple{0b0000011, 0b000, 0},         LB},
            {Triple{0b0000011, 0b001, 0},         LH},
            {Triple{0b0000011, 0b010, 0},         LW},
            {Triple{0b0000011, 0b100, 0},         LBU},
            {Triple{0b0000011, 0b101, 0},         LHU},
            //S-type
            {Triple{0b0100011, 0b000, 0},         SB},
            {Triple{0b0100011, 0b001, 0},         SH},
            {Triple{0b0100011, 0b010, 0},         SW},
            //I-type
            {Triple{0b0010011, 0b000, 0},         ADDI},
            {Triple{0b0010011, 0b010, 0},         SLTI},
            {Triple{0b0010011, 0b011, 0},         SLTIU},
            {Triple{0b0010011, 0b100, 0},         XORI},
            {Triple{0b0010011, 0b110, 0},         ORI},
            {Triple{0b0010011, 0b111, 0},         ANDI},
            //I-type no register-id, only immediate value, may use for function/shift-amount
            //fake func7 ?
            {Triple{0b0010011, 0b001, 0b0000000}, SLLI},
            {Triple{0b0010011, 0b101, 0b0000000}, SRLI},
            {Triple{0b0010011, 0b101, 0b0100000}, SRAI},
            //R-type
            {Triple{0b0110011, 0b000, 0b0000000}, ADD},
            {Triple{0b0110011, 0b000, 0b0100000}, SUB},
            {Triple{0b0110011, 0b001, 0b0000000}, SLL},
            {Triple{0b0110011, 0b010, 0b0000000}, SLT},
            {Triple{0b0110011, 0b011, 0b0000000}, SLTU},
            {Triple{0b0110011, 0b100, 0b0000000}, XOR},
            {Triple{0b0110011, 0b101, 0b0000000}, SRL},
            {Triple{0b0110011, 0b101, 0b0100000}, SRA},
            {Triple{0b0110011, 0b110, 0b0000000}, OR},
            {Triple{0b0110011, 0b111, 0b0000000}, AND}
    };

    Ins Decode(u_int ins_code) {
        auto P = [=](uint32_t r, uint32_t l) { return Pick(ins_code, r, l); };
        auto PIMM = [=](uint32_t r, uint32_t l, uint32_t k) { return Pick(ins_code, r, l) << k; };
        Ins ret;
        ret.opcode = P(6, 0);
        switch (code_map[ret.opcode]) {
            case R_type:
                ret.rd = P(11, 7);
                ret.rs1 = P(19, 15);
                ret.rs2 = P(24, 20);
                ret.fun3 = P(14, 12);
                ret.fun7 = P(31, 25);
                break;
            case I_type:
                ret.rd = P(11, 7);
                ret.rs1 = P(19, 15);
                ret.fun3 = P(14, 12);
                if (ret.opcode == 0b0010011 && (ret.fun3 == 0b001 || ret.fun3 == 0b101)) {
                    ret.fun7 = P(31, 25);// I-type can have func7 ?
                    ret.shamt = P(24, 20);
                }
                ret.imm = SExt(PIMM(31, 20, 0), 11);
                break;
            case S_type:
                ret.rs1 = P(19, 15);
                ret.rs2 = P(24, 20);
                ret.fun3 = P(14, 12);
                ret.imm = SExt(PIMM(31, 25, 5) + PIMM(11, 7, 0), 11);
                break;
            case B_type:
                ret.rs1 = P(19, 15);
                ret.rs2 = P(24, 20);
                ret.fun3 = P(14, 12);
                ret.imm = PIMM(31, 31, 12) + PIMM(30, 25, 5) + PIMM(11, 8, 1) + PIMM(7, 7, 11);
                ret.imm = SExt(ret.imm, 12);
                break;
            case U_type:
                ret.rd = P(11, 7);
                ret.imm = PIMM(31, 12, 12);
                break;
            case J_type:
                ret.rd = P(11, 7);
                ret.imm = PIMM(31, 31, 20) + PIMM(30, 21, 1) + PIMM(20, 20, 11) + PIMM(19, 12, 12);
                ret.imm = SExt(ret.imm, 20);
                break;
        }
        ret.ins_type = ins_map[Triple{ret.opcode, ret.fun3, ret.fun7}];
        ret.rd = P(11, 7);
        ret.rs1 = P(19, 15);
        ret.rs2 = P(24, 20);
        ret.fun3 = P(14, 12);
        ret.fun7 = P(31, 25);
        return ret;
    }
}

#endif
