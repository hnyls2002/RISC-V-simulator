//
// Created by m1375 on 2022/6/28.
//

#ifndef RISC_V_SIMULATOR_SIMULATOR_HPP
#define RISC_V_SIMULATOR_SIMULATOR_HPP

#include <iostream>
#include <algorithm>
#include <random>
#include <bitset>
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
            uint32_t pc{}, reg[32]{};
            RegFile reg_file[32]{};
            Queue<ROB_Ins, 32> rob;
            ResStat<32> res_st{};
            LSBuffer<32> ls_buffer{};
            CDB_Type cdb1{}, cdb2{};// 可能同时会有Execute 和 Memory Access 完成，增加CDB的带宽
        } input, now;
        Memory<500000> memory;

        // the issue wires
        bool issue_flag{};// 是否发射成功
        Ins issue_ins{};
        int issue_id{};

        // memory wires
        bool no_access{};

        // 0ff00513
        bool end_flag{};

        int clk{}, branch_total{}, branch_correct{};
        std::bitset<4> branch_history[1024];
        std::bitset<2> pattern_counter[16];

    public:

        void Init() {
            std::string info;
            uint32_t addr = 0, offset;
            while (std::cin >> info) {
                if (info[0] == '@')addr = HexStrToInt(info.substr(1, 8)), offset = 0;
                else memory[(int) (addr + offset++)] = HexStrToInt(info);
            }
        }

        void Run_RS() {
            // 指令发射成功以及是发送到RS中的指令
            if (issue_flag && RSType[issue_ins.ins_type] <= BRC) {
                RS_Ins rs_node{};
                rs_node.busy = true;
                rs_node.A = issue_ins.imm;
                rs_node.op = issue_ins.ins_type;
                rs_node.id = issue_id;
                rs_node.ins_pc = input.pc;

                if (issue_ins.code_type == R_type || issue_ins.code_type == B_type) {
                    Fetch_Reg(issue_ins.rs1, rs_node.V1, rs_node.Q1);
                    Fetch_Reg(issue_ins.rs2, rs_node.V2, rs_node.Q2);
                } else if (issue_ins.code_type == I_type)
                    Fetch_Reg(issue_ins.rs1, rs_node.V1, rs_node.Q1);

                now.res_st.Insert(rs_node);// 放进RS中
            }

        }

        void Run_CDB() {
            for (auto cdb: {input.cdb1, input.cdb2}) {
                if (!cdb.busy)continue;
                for (int i = 0; i < 32; ++i)// check RS
                    if (input.res_st[i].busy) {
                        if (input.res_st[i].Q1 == cdb.id)now.res_st[i].Q1 = 0, now.res_st[i].V1 = cdb.val;
                        if (input.res_st[i].Q2 == cdb.id)now.res_st[i].Q2 = 0, now.res_st[i].V2 = cdb.val;
                    }
                for (int i = 0; i < 32; ++i)// check LSB
                    if (input.ls_buffer[i].busy) {
                        if (input.ls_buffer[i].Q1 == cdb.id)now.ls_buffer[i].Q1 = 0, now.ls_buffer[i].V1 = cdb.val;
                        if (input.ls_buffer[i].Q2 == cdb.id)now.ls_buffer[i].Q2 = 0, now.ls_buffer[i].V2 = cdb.val;
                    }
                // check ROB
                int id = (int) cdb.id;
                now.rob[id].ready = true;
                if (now.rob[id].des == 0)now.rob[id].val = 0;
                else now.rob[id].val = cdb.val;
                now.rob[id].jump_real = cdb.jump;
                now.rob[id].n_pc = cdb.n_pc;
            }
            now.cdb1.busy = now.cdb2.busy = false;
        }

        void Run_LSB() {
            // 指令发射成功 and 是发送到LSB中的指令
            if (issue_flag && RSType[issue_ins.ins_type] >= LD) {
                LSB_Ins lsb_node{};
                lsb_node.busy = true;
                lsb_node.calc_done = false;
                lsb_node.A = issue_ins.imm;
                lsb_node.op = issue_ins.ins_type;
                lsb_node.id = issue_id;
                lsb_node.ins_clk = clk;
                lsb_node.ls_type = RSType[issue_ins.ins_type];
                lsb_node.cmt_flag = false;
                if (lsb_node.ls_type == ST) {// rs1 rs2
                    Fetch_Reg(issue_ins.rs1, lsb_node.V1, lsb_node.Q1);
                    Fetch_Reg(issue_ins.rs2, lsb_node.V2, lsb_node.Q2);
                } else Fetch_Reg(issue_ins.rs1, lsb_node.V1, lsb_node.Q1);

                now.ls_buffer.Insert(lsb_node);// 放进LSB中
            }

            static int lsb_store_pos_msg;
            static LSB_Ins ins;
            lsb_store_pos_msg = input.ls_buffer.Find_Store_Ready();
            if (lsb_store_pos_msg != -1) {// STORE指令的两个寄存器计算完成，发送给ROB表明ready
                ins = input.ls_buffer[lsb_store_pos_msg];
                now.rob[(int) ins.id].ready = true;
                now.ls_buffer[lsb_store_pos_msg].calc_done = true;
            }

            // LSB 找到可以进行内存访问的 由commit来调用
        }

        void Run_ROB() {
            // ROB 模块
            // 接收到 issue 组合逻辑发出的指令，同时进行分支预测（pc的改动在ROB中）
            if (issue_flag) {
                auto rs_type = RSType[issue_ins.ins_type];
                if (rs_type == BSC || rs_type >= LD) {
                    now.pc = input.pc + 4;
                    now.rob.Push(ROB_Ins(issue_ins, false, input.pc));
                } else {
                    if (issue_ins.ins_type == JAL) {
                        now.pc = input.pc + issue_ins.imm;// pc + offset
                        now.rob.Push(ROB_Ins(issue_ins, true, input.pc));
                    } else if (issue_ins.ins_type == JALR) { // JALR 直接下一个，到时候回滚
                        now.pc = input.pc + 4;
                        now.rob.Push(ROB_Ins(issue_ins, false, input.pc));
                    } else {
                        // Predict Here...
                        uint32_t his = branch_history[input.pc & 1023u].to_ulong();
                        if (pattern_counter[his].test(1)) {// jump status
                            now.pc = input.pc + issue_ins.imm;
                            now.rob.Push(ROB_Ins(issue_ins, true, input.pc));
                        } else {
                            now.pc = input.pc + 4;
                            now.rob.Push(ROB_Ins(issue_ins, false, input.pc));
                        }
                    }
                }
            }
        }

        void Run_RegFile() {
            if (!issue_flag)return;// 必须要发射成功
            bool flag = false;
            switch (RSType[issue_ins.ins_type]) {
                case BSC:
                case LD:
                case JUMP:
                    if (issue_ins.rd == 0) {
                        now.reg_file[0].busy = false;
                        now.reg_file[issue_ins.rd].id = 0;
                    } else {
                        now.reg_file[issue_ins.rd].busy = true;
                        now.reg_file[issue_ins.rd].id = issue_id;
                    }
                    break;
                default:
                    // Store 和 Branch 指令没有目标寄存器
                    break;
            }
        }

        void Update() {// 更新线路？
            ++clk;
            input = now;
        }

        void Fetch_Reg(uint32_t rs, uint32_t &V, uint32_t &Q) {// RS/LSB调用，从其他部件中Fetch
            if (rs == 0) V = Q = 0;
            else if (!input.reg_file[rs].busy) V = input.reg[rs], Q = 0;
            else {
                int id = (int) input.reg_file[rs].id;
                if (input.rob[id].ready) V = input.rob[id].val, Q = 0;
                else if (input.cdb1.busy && input.cdb1.id == id) V = input.cdb1.val, Q = 0;
                else if (input.cdb2.busy && input.cdb2.id == id) V = input.cdb2.val, Q = 0;
                else Q = id;
            }
        }

        void Issue() {
            // 读取过终止指令了，不用再读取了
            if (end_flag)return;

            if (!now.rob.Ava()) {
                issue_flag = false;
                return;
            }


            issue_id = now.rob.AvaPos();// 发送 rob 空余的位置
            issue_ins = Decode(memory.ReadMem((int) now.pc, 3));// 发送解析出来的指令
            issue_flag = false;

            auto rs_type = RSType[issue_ins.ins_type];
            if (rs_type <= BRC && now.res_st.Ava()) {
                if (memory.ReadMem((int) now.pc, 3) == 0x0ff00513)end_flag = true;
                issue_flag = true;
            }
            if (rs_type >= LD && now.ls_buffer.Ava())issue_flag = true;

        }

        void Commit() {
            if (now.rob.Empty() || !now.rob.Front().ready)return;

            auto id = now.rob.TopPos();
            auto ins = now.rob.Pop();

            auto rs_type = RSType[ins.ins_type];
            switch (rs_type) {
                case BSC:
                case LD:
                case JUMP:
                    // commit成功了要更新regfile，注意issue阶段可能也修改了regfile
                    // 精确终端，必须给reg先赋值，即使后面的操作由重新定义了reg的值（因为后面的操作可能假了）
                    now.reg[ins.des] = ins.val;
                    if (now.reg_file[ins.des].id == id && now.reg_file[ins.des].busy) {
                        now.reg_file[ins.des].busy = false;
                        now.reg_file[ins.des].id = 0;
                    }
                    break;
                default:
                    // Store 和 Branch 指令没有目标寄存器
                    break;

            }
            if (rs_type == ST) {// 需要修改LSB中的cmt状态
                for (int i = 0; i < 32; ++i)
                    if (now.ls_buffer[i].id == id) {
                        now.ls_buffer[i].cmt_flag = true;
                        break;
                    }
            }
            if (rs_type == BRC || ins.ins_type == JALR) { // 判断分支预测的准确性
                if (rs_type == BRC) {
                    ++branch_total;
                    if (ins.jump_real == ins.jump_prdc)++branch_correct;
                    auto &his = branch_history[ins.ins_pc & 1023u];
                    auto las = his;
                    his = his << 1, his[0] = ins.jump_real;
                    auto &c = pattern_counter[las.to_ulong()];

                    switch (c.to_ulong()) {
                        case 0b00:
                            c = ins.jump_real ? 0b01 : 0b00;
                            break;
                        case 0b01:
                            c = ins.jump_real ? 0b10 : 0b00;
                            break;
                        case 0b10:
                            c = ins.jump_real ? 0b11 : 0b01;
                            break;
                        case 0b11:
                            c = ins.jump_real ? 0b11 : 0b10;
                            break;
                    }

                }
                if (ins.jump_prdc != ins.jump_real) {// 预测错误
                    RollBack(ins.n_pc);
                }
            }
/*
            printf("id = %d\n", id);
            printf("pc = %d\n", ins.ins_pc);
            printf("Ins Type : %s\n", InsStr[ins.ins_type].c_str());
            printf("rs1(%d)\trs2(%d)\timm(%d)\trd(%d)\n", ins.rs1, ins.rs2, ins.imm, ins.des);
            printf("val = %d\n", ins.val);
            for (int i = 0; i < 32; ++i)
                printf("reg[%d] = %d\n", i, (int) now.reg[i]);
            printf("------------------\n");
*/

        }

        void RollBack(uint32_t n_pc) {
            now.pc = n_pc;
            while (!no_access) {
                ++clk;
                Memory_Access();
            }
            now.ls_buffer.Clear();
            now.rob.Clear();
            now.res_st.Clear();
            now.cdb1.busy = false;
            now.cdb2.busy = false;
            for (int i = 0; i < 32; ++i)
                now.reg_file[i].busy = false;
        }

        void Execute() {
            // Pick 一条 Ready 的指令发送给ALU
            int rs_pos_msg = now.res_st.Find_Ready();

            if (rs_pos_msg != -1) {// 指令发送成功
                auto ins = now.res_st[rs_pos_msg];
                uint32_t val = 0, n_pc = 0;
                bool jump = false;
                switch (ins.op) {
                    case ADD:
                        val = ins.V1 + ins.V2;
                        break;
                    case ADDI:
                        val = ins.V1 + ins.A;
                        break;
                    case SUB:
                        val = ins.V1 - ins.V2;
                        break;
                    case LUI:
                        val = ins.A;
                        break;
                    case AUIPC:
                        val = ins.ins_pc + ins.A;
                        break;
                    case XOR:
                        val = ins.V1 ^ ins.V2;
                        break;
                    case XORI:
                        val = ins.V1 ^ ins.A;
                        break;
                    case OR:
                        val = ins.V1 | ins.V2;
                        break;
                    case ORI:
                        val = ins.V1 | ins.A;
                        break;
                    case AND:
                        val = ins.V1 & ins.V2;
                        break;
                    case ANDI:
                        val = ins.V1 & ins.A;
                        break;
                    case SLL:
                        val = ins.V1 << (ins.V2 & 31);
                        break;
                    case SLLI:
                        val = ins.V1 << ins.A;
                        break;
                    case SRL:
                        val = ins.V1 >> (ins.V2 & 31);
                        break;
                    case SRLI:
                        val = ins.V1 >> ins.A;
                        break;
                    case SRA:
                        val = ((int32_t) ins.V1) >> (ins.V2 & 31);
                        break;
                    case SRAI:
                        val = ((int32_t) ins.V1) >> ins.A;
                        break;
                    case SLT:// signed less than
                        val = ((int32_t) ins.V1) < ((int32_t) ins.V2) ? 1 : 0;
                        break;
                    case SLTI:
                        val = ((int32_t) ins.V1) < ((int32_t) ins.A) ? 1 : 0;
                        break;
                    case SLTU:
                        val = ins.V1 < ins.V2 ? 1 : 0;
                        break;
                    case SLTIU:
                        val = ins.V1 < ins.A ? 1 : 0;
                        break;
                    case BEQ:
                        if (ins.V1 == ins.V2)jump = true, n_pc = ins.ins_pc + ins.A;
                        else jump = false, n_pc = ins.ins_pc + 4;
                        break;
                    case BNE:
                        if (ins.V1 != ins.V2)jump = true, n_pc = ins.ins_pc + ins.A;
                        else jump = false, n_pc = ins.ins_pc + 4;
                        break;
                    case BLT:
                        if (((int32_t) ins.V1) < ((int32_t) ins.V2))jump = true, n_pc = ins.ins_pc + ins.A;
                        else jump = false, n_pc = ins.ins_pc + 4;
                        break;
                    case BGE:
                        if (((int32_t) ins.V1) >= ((int32_t) ins.V2))jump = true, n_pc = ins.ins_pc + ins.A;
                        else jump = false, n_pc = ins.ins_pc + 4;
                        break;
                    case BLTU:
                        if (ins.V1 < ins.V2)jump = true, n_pc = ins.ins_pc + ins.A;
                        else jump = false, n_pc = ins.ins_pc + 4;
                        break;
                    case BGEU:
                        if (ins.V1 >= ins.V2)jump = true, n_pc = ins.ins_pc + ins.A;
                        else jump = false, n_pc = ins.ins_pc + 4;
                        break;
                    case JAL:
                        val = ins.ins_pc + 4;
                        break;
                    case JALR:
                        val = ins.ins_pc + 4;
                        n_pc = (ins.V1 + ins.A) & ~1;
                        jump = true;
                        break;
                    default:
                        throw UnexpectedInstruction();
                }
                now.cdb1 = {true, jump, ins.id, val, n_pc};
                now.res_st[rs_pos_msg].busy = false;
            }
        }

        void Memory_Access() {
            // 进行 memory_access 组合逻辑，但是消耗三个周期
            static bool in_memory_access = false; // 使用第二条cdb
            static int beg_clk = 0, lsb_memory_pos_msg = 0;
            static uint32_t val = 0;
            static LSB_Ins ins{};

            if (!in_memory_access) {// 目前没有在进行内存访问
                lsb_memory_pos_msg = now.ls_buffer.Find_Memory_Ready();// 组合逻辑，直接用当前的值
                if (lsb_memory_pos_msg != -1) {
                    ins = now.ls_buffer[lsb_memory_pos_msg];
                    in_memory_access = true;
                    beg_clk = clk;
                    no_access = false;
                } else no_access = true;
            }
            if (in_memory_access && clk - beg_clk == 2) {
                switch (ins.op) {
                    case LB:
                        val = SExt(memory.ReadMem((int) (ins.V1 + ins.A), 1), 15);
                        break;
                    case LW:
                        val = memory.ReadMem((int) (ins.V1 + ins.A), 3);
                        break;
                    case LBU:
                        val = memory.ReadMem((int) (ins.V1 + ins.A), 0);
                        break;
                    case LHU:
                        val = memory.ReadMem((int) (ins.V1 + ins.A), 1);
                        break;
                    case SB:
                        memory.WriteMem(ins.V2, (int) (ins.V1 + ins.A), 0);
                        break;
                    case SH:
                        memory.WriteMem(ins.V2, (int) (ins.V1 + ins.A), 1);
                        break;
                    case SW:
                        memory.WriteMem(ins.V2, (int) (ins.V1 + ins.A), 3);
                        break;
                    default:
                        throw UnexpectedInstruction();
                }
                if (RSType[ins.op] == LD) {
                    now.cdb2 = {true, false, ins.id, val, 0};
                }
                now.ls_buffer[lsb_memory_pos_msg].busy = false;
                in_memory_access = false;
            }
        }

        void (Simulator::*comp[5])() = {&Simulator::Run_ROB, &Simulator::Run_RS, &Simulator::Run_RegFile,
                                        &Simulator::Run_CDB, &Simulator::Run_LSB};

        void Run() {
            srand((unsigned) time(NULL));
            while (true) {
                Update();
                std::random_shuffle(comp, comp + 4);

//                std::shuffle(comp, comp + 4, std::mt19937(std::random_device()()));

                for (int i = 0; i < 5; ++i)(this->*comp[i])();
/*
                Run_CDB();
                Run_LSB();
                Run_ROB();
                Run_RS();
                Run_RegFile();
*/

                Execute();
                Memory_Access();
                Commit();
                Issue();

                if (end_flag && now.rob.Empty()) {
                    std::cout << std::dec << (now.reg[10] & ((uint32_t) 255)) << std::endl;
                    std::cerr << "\033[1m\033[37mAns is " << std::dec << (now.reg[10] & ((uint32_t) 255)) << std::endl;
                    break;
                }
            }
//            std::cout << "fuck" << std::endl;
/*
            std::cout << branch_correct << "/" << branch_total << " = " << (double) branch_correct / branch_total
                      << std::endl;
*/
        }
    };
}

#endif //RISC_V_SIMULATOR_SIMULATOR_HPP
