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
        uint32_t Q;
    };

    struct Rnm_Ins {// Rename-Instruction for reservation-station & load-store-buffer
        bool busy;
        Ins_ENUM op;
        uint32_t V1, V2, Q1, Q2, A;
    };

    template<const size_t SIZE = 32>
    class ResStat {
    private:
        Rnm_Ins data[SIZE];

    public:
        bool Ava() {
            for (int i = 0; i < SIZE; ++i)
                if (data[i].busy)return false;
            return true;
        }

        int Insert(const Rnm_Ins &val) {
            for (int i = 0; i < SIZE; ++i) {
                if (!data[i].busy) {
                    data[i] = val;
                    return i;
                }
            }
            throw BufferFullException();
        }
    };

    template<const size_t SIZE = 32>
    class LSBuffer {
    private:
        Rnm_Ins data[SIZE];
        int iss_clk[SIZE];

    public:
        bool Ava() {
            for (int i = 0; i < SIZE; ++i)
                if (data[i].busy)return false;
            return true;
        }

        int Insert(const Rnm_Ins &val, int c) {
            for (int i = 0; i < SIZE; ++i) {
                if (!data[i].busy) {
                    data[i] = val, iss_clk[i] = c;
                    return i;
                }
            }
            throw BufferFullException();
        }
    };

    struct ROB_Ins {
        u_int32_t opcode{}, des{}, rs1{}, rs2{}, fun3{}, fun7{}, imm{}, shamt{};// shift amount
        Ins_ENUM ins_type{};
    };

    template<class T, const int MAX_SIZE>
    class Queue {
    private:
        T qu[MAX_SIZE];
        int hd{1}, tl{0};

    public:
        size_t Size() const { return tl - hd + 1; }

        bool Full() const { return Size() == MAX_SIZE; }

        bool Ava() const { return Size() != MAX_SIZE; }

        bool Empty() const { return hd == tl + 1; }

        void Push(const T &data) {
            if (Size() == MAX_SIZE)throw BufferFullException();
            qu[(++tl) % MAX_SIZE] = data;
        }

        T Front() const {
            if (Empty())throw BufferEmptyException();
            return qu[hd % MAX_SIZE];
        }

        T Pop() {
            if (Empty())throw BufferEmptyException();
            return qu[(hd++) % MAX_SIZE];
        }

        T &operator[](const int &k) {
            if (k >= MAX_SIZE || k < -MAX_SIZE)throw IndexOutException();
            return qu[(k + MAX_SIZE) % MAX_SIZE];
        }

        void Clear() { hd = tl + 1; }

    };

}


#endif //RISC_V_SIMULATOR_COMPONENTS_HPP
