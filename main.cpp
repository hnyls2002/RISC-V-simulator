#include <iostream>
#include "Simulator.hpp"

using namespace hnyls2002;

#define DIR "testcases_for_riscv/"

int main() {
/*
    freopen(DIR"testcases/pi.data", "r", stdin);
    freopen("test.out", "w", stdout);
*/
    Simulator sim;
    sim.Init();
    sim.Run();
    return 0;
}