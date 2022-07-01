//
// Created by m1375 on 2022/6/28.
//

#ifndef RISC_V_SIMULATOR_COMPONENTS_HPP
#define RISC_V_SIMULATOR_COMPONENTS_HPP

#include <iostream>
#include "Exception.hpp"
#include "Decode.hpp"

namespace hnyls2002 {
    template<const size_t SIZE = 500000>
    class Memory {
    private:
        uint32_t data[SIZE]{};

    public:
        uint32_t ReadMem(int p, int k) {// k range in [3,0]
            uint32_t ret = 0;
            for (int i = p + k; i >= p; --i)
                ret = (ret << 8) + data[i];
            return ret;
        }

        void WriteMem(uint32_t x, int p, int k) {// write and read are both little-endian
            for (int i = p; i <= p + k; ++i)
                data[i] = x & 255, x >>= 8;
        }

        uint32_t &operator[](const int &k) {
            return data[k];
        }
    };

    struct RegFile {
        bool busy;
        uint32_t id;// id in reorder buffer, always cover
    };

    struct RS_Ins {// Rename-Instruction for reservation-station & load-store-buffer
        bool busy;
        Ins_ENUM op;
        uint32_t ins_pc, V1, V2, Q1, Q2, A, id;// id in reorder buffer , for commit
    };

    template<const size_t SIZE = 32>
    class ResStat {
    private:
        RS_Ins data[SIZE];
    public:

        void Clear() {
            for (int i = 0; i < SIZE; ++i) data[i].busy = false;
        }

        RS_Ins &operator[](const int &k) {
            return data[k];
        }

        bool Ava() {
            for (int i = 0; i < SIZE; ++i)
                if (data[i].busy)return false;
            return true;
        }

        int Insert(const RS_Ins &val) {
            for (int i = 0; i < SIZE; ++i) {
                if (!data[i].busy) {
                    data[i] = val;
                    return i;
                }
            }
            throw BufferFullException();
        }

        int Find_Ready() {
            for (int i = 0; i < SIZE; ++i)
                if (data[i].busy && data[i].Q1 == 0 && data[i].Q2 == 0)
                    return i;
            return -1;
        }

    };

    struct LSB_Ins {// Rename-Instruction for reservation-station & load-store-buffer
        bool busy, cmt_flag, calc_done;
        Ins_ENUM op;
        uint32_t V1, V2, Q1, Q2, A, id;// id in reorder buffer , for commit
        int ins_clk;
        RS_ENUM ls_type;
    };

    template<const size_t SIZE = 32>
    class LSBuffer {
//    private:
        LSB_Ins data[SIZE];

    public:

        void Clear() {
            for (int i = 0; i < SIZE; ++i) data[i].busy = false;
        }

        LSB_Ins &operator[](const int &k) {
            return data[k];
        }

        bool Ava() {
            for (int i = 0; i < SIZE; ++i)
                if (data[i].busy)return false;
            return true;
        }

        int Insert(const LSB_Ins &val) {
            for (int i = 0; i < SIZE; ++i) {
                if (!data[i].busy) {
                    data[i] = val;
                    return i;
                }
            }
            throw BufferFullException();
        }

        int Find_Store_Ready() {
            for (int i = 0; i < SIZE; ++i)
                if (data[i].busy && !data[i].calc_done && data[i].ls_type == ST && data[i].Q1 == 0 && data[i].Q2 == 0)
                    return i;
            return -1;
        }

        int Find_Memory_Ready() { // the min clk
            int Min = 0x3f3f3f3f, ind = -1;
            for (int i = 0; i < SIZE; ++i)
                if (data[i].busy && data[i].ins_clk < Min)
                    Min = data[i].ins_clk, ind = i;
            if (ind == -1)return -1;
            if (data[ind].ls_type == LD && data[ind].Q1 == 0) return ind;
            if (data[ind].ls_type == ST && data[ind].cmt_flag) return ind;
            return -1;
        }
    };

    struct ROB_Ins {
        bool ready{}, jump_prdc{}, jump_real{};
        Ins_ENUM ins_type{};
        uint32_t des{}, rs1{}, rs2{}, imm{}, val{}, n_pc{}, ins_pc{};

        ROB_Ins() = default;

        explicit ROB_Ins(const Ins &ins, bool _jump_prdc, uint32_t now_pc) : ready(false), jump_prdc(_jump_prdc),
                                                                             ins_type(ins.ins_type), des(ins.rd),
                                                                             rs1(ins.rs1), rs2(ins.rs2), imm(ins.imm),
                                                                             val(0), ins_pc(now_pc) {}
    };

    template<class T, const int MAX_SIZE>
    class Queue {
    private:
        T qu[MAX_SIZE]{};

    public:
        int hd{1}, tl{0};

        size_t Size() const { return tl - hd + 1; }

        bool Ava() const { return Size() != MAX_SIZE; }

        int AvaPos() const { return tl + 1; }

        bool Empty() const { return hd == tl + 1; }

        int Push(const T &data) {
            if (Size() == MAX_SIZE)throw BufferFullException();
            qu[(++tl) % MAX_SIZE] = data;
            return tl;
        }

        T Front() const {
            if (Empty())throw BufferEmptyException();
            return qu[hd % MAX_SIZE];
        }

        int TopPos() const { return hd; }

        T Pop() {
            if (Empty())throw BufferEmptyException();
            return qu[(hd++) % MAX_SIZE];
        }

        T &operator[](const int &k) {
            if (k > tl || k < hd) throw IndexOutException();
            return qu[k % MAX_SIZE];
        }

        void Clear() { tl = hd - 1; }

    };

    struct CDB_Type {
        bool busy, jump;
        uint32_t id, val, n_pc;// 算出来的val放入id号ROB中
    };
}


#endif //RISC_V_SIMULATOR_COMPONENTS_HPP
