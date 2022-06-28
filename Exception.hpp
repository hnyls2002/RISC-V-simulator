//
// Created by m1375 on 2022/6/28.
//

#ifndef RISC_V_SIMULATOR_EXCEPTION_HPP
#define RISC_V_SIMULATOR_EXCEPTION_HPP

#include <exception>

namespace hnyls2002 {
    class BufferFullException : public std::exception {
    };

    class BufferEmptyException : public std::exception {
    };

    class IndexOutException : public std::exception {
    };

    class UnexpectedInstruction : public std::exception {
    };
};

#endif //RISC_V_SIMULATOR_EXCEPTION_HPP
