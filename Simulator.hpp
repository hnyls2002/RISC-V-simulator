//
// Created by m1375 on 2022/6/28.
//

#ifndef RISC_V_SIMULATOR_SIMULATOR_HPP
#define RISC_V_SIMULATOR_SIMULATOR_HPP

#include <iostream>
#include "Components.hpp"

namespace hnyls2002 {

    uint32_t HexStrToInt(const std::string &str) {
        uint32_t ret = 0;
        for (auto ch: str)
            ret = (ret << 4) + (ch <= '9' ? ch - '0' : ch - 'A' + 10);
        return ret;
    }

    class Simulator {
    private:
        struct HardWare {
            Memory<500000> memory;
            uint32_t pc{}, reg[32]{};
            RegFile reg_file[32]{};
            Queue<uint32_t, 32> ins_que{};
            Queue<std::pair<Ins, uint32_t>, 32> rob{};// second -> value
            ResStat<32> res_st{};
            LSBuffer<32> ls_buffer{};
        } las, nex;
        int clk{};
    public:

        void Init() {
            std::string info;
            uint32_t addr = 0, offset;
            while (std::cin >> info) {
                if (info[0] == '@')addr = HexStrToInt(info.substr(1, 8)), offset = 0;
                else las.memory[addr + offset++] = HexStrToInt(info);
            }
            nex = las;
        }

        void Run_InsQue() {
            if (las.ins_que.Ava()) {
                las.ins_que.Push(las.memory.ReadMem(las.pc, 3));
                nex.pc = las.pc + 4;
            }
        }

        void Run_RS() {
        }

        void Run_LSB() {
        }

        void Run_ROB() {
        }

        void Update() {
        }

        void Issue() {
            if (!las.rob.Ava())return;// rob 没有空余的位置

            Ins ins = Decode(las.ins_que.Front());
            if (RSType[ins.ins_type] == BSC) {
                if (!las.res_st.Ava())return;// reservation station 没有空余的位置

                Rnm_Ins node{};
                node.busy = true;
                node.A = ins.imm;
                node.op = ins.ins_type;

                switch (code_map[ins.opcode]) {
                    case R_type:// rs1 rs2 rd
                        if (las.reg_file[ins.rs2].busy)node.Q2 = las.reg_file[ins.rs2].Q;
                        else node.V2 = las.reg[ins.rs2];
                    case I_type:// rs1 rd
                        if (las.reg_file[ins.rs1].busy)node.Q1 = las.reg_file[ins.rs1].Q;
                        else node.V1 = las.reg[ins.rs1];
                    case U_type: // rd
                    case J_type:// rd
                        break;
                    default:// 不可能是B-type和S-type
                        throw UnexpectedInstruction();
                }
                nex.res_st.Insert(node);
                nex.rob.Push({ins, 0});
            }
        }

        void Commit() {
        }

        void Execute() {
        }
    };
}

#endif //RISC_V_SIMULATOR_SIMULATOR_HPP
