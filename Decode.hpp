//
// Created by m1375 on 2022/6/28.
//

#ifndef RISC_V_SIMULATOR_DECODE_HPP
#define RISC_V_SIMULATOR_DECODE_HPP

#include <map>
#include <tuple>

namespace hnyls2002 {

    enum Ins_ENUM {
        LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU, LB, LH, LW, LBU, LHU, SB, SH, SW, ADDI,
        SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
    };

    const std::string InsStr[] = {// for debug
            "LUI", "AUIPC", "JAL", "JALR", "BEQ", "BNE", "BLT", "BGE", "BLTU",
            "BGEU", "LB", "LH", "LW", "LBU", "LHU", "SB", "SH", "SW", "ADDI",
            "SLTI", "SLTIU", "XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI", "ADD",
            "SUB", "SLL", "SLT", "SLTU", "XOR", "SRL", "SRA", "OR", "AND"
    };

    enum RS_ENUM {
        BSC, JUMP, BRC, LD, ST
    };

    const RS_ENUM RSType[] = {
            BSC, BSC, JUMP, JUMP, BRC, BRC, BRC, BRC, BRC, BRC, LD, LD,
            LD, LD, LD, ST, ST, ST, BSC, BSC, BSC, BSC, BSC, BSC,
            BSC, BSC, BSC, BSC, BSC, BSC, BSC, BSC, BSC, BSC, BSC, BSC, BSC
    };

    enum Code_ENUM {
        R_type, I_type, S_type, B_type, U_type, J_type
    };

    struct Ins {
        Ins_ENUM ins_type{};
        Code_ENUM code_type{};
        u_int32_t rd{}, rs1{}, rs2{}, imm{};// (shift amount)
    };

    uint32_t Pick(uint32_t x, uint32_t r, uint32_t l) {// [r:l] -> [r-l:0]
        return x >> l & ((1 << (r - l + 1)) - 1);
    }

    u_int32_t SExt(uint32_t x, uint32_t h) { // using the highest bit to extend
        if (1 << h & x)x = x | (-(1 << h));
        return x;
    }

    std::map<u_int32_t, Code_ENUM> code_map = {
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

    std::map<Triple, Ins_ENUM> ins_map = {
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
        uint32_t opcode = 0, fun3 = 0, fun7 = 0;
        opcode = P(6, 0);
        ret.code_type = code_map[opcode];
        switch (ret.code_type) {
            case R_type:
                ret.rd = P(11, 7);
                ret.rs1 = P(19, 15);
                ret.rs2 = P(24, 20);
                fun3 = P(14, 12);
                fun7 = P(31, 25);
                break;
            case I_type:
                ret.rd = P(11, 7);
                ret.rs1 = P(19, 15);
                fun3 = P(14, 12);
                if (opcode == 0b0010011 && (fun3 == 0b001 || fun3 == 0b101)) {
                    fun7 = P(31, 25);// I-type can have func7 ?
                    ret.imm = P(24, 20);
                } else ret.imm = SExt(PIMM(31, 20, 0), 11);
                break;
            case S_type:
                ret.rs1 = P(19, 15);
                ret.rs2 = P(24, 20);
                fun3 = P(14, 12);
                ret.imm = SExt(PIMM(31, 25, 5) + PIMM(11, 7, 0), 11);
                break;
            case B_type:
                ret.rs1 = P(19, 15);
                ret.rs2 = P(24, 20);
                fun3 = P(14, 12);
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
        ret.ins_type = ins_map[Triple{opcode, fun3, fun7}];
        return ret;
    }

}

#endif //RISC_V_SIMULATOR_DECODE_HPP
