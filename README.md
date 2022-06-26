# RISC-V-simulator

Tomasulo

### 部件设计

- Instruction Queue
- 31 General-purpose register and x0 register
- pc register
- Reservation Station (for every ALU)
- Load & Store Buffer
- ALU(**combinational logic** in RS or LSB)
- CDB(**sequential logic**)
- Register File (Indicating every register's status : busy or not/how to get its value in ROB)
- Reorder Buffer
- Memory Unit

### 时序逻辑 & 组合逻辑

- 组合逻辑任意时刻的输出仅仅取决该时刻的输入，与时钟无关。
- 时序逻辑先计算好输入的结果，在下一个周期的上升沿才会把结果更新到部件，此处的结果是上个周期另一个部件的输出。

### Hazards Solution

所有的write指令全部被rename

- WAW hazard：两个write操作，rd相同的情况下，由于指令是顺序issue的，第二个指令会issue的时候会覆盖掉register status的装填，标记为第二个指令的rename结果，于是第一个指令就算后执行完，也并不会和register status匹配。
- WAR hazard：先read后write，由于Reservation Station中直接存储了register的value（an immediate），所以之后再与从哪个register中读取数据无关了，第二个write操作想怎么弄，就怎么弄。
- RAW hazard：先write后read，第一条指令issue之后rename为Reservation Station的编号，于是第二条指令issue的时候检查Register Status，发现这个寄存器被rename了，于是真正的依赖关系产生了。

### Load & Store

- Load 和 Store 要求从内存中读取数据，内存和寄存器一样，也是存储数据的，所以也会有hazard出现。
- 对于Load 和 Store操作，建立Load Store Buffer，相当于普通操作的Reservation Station。

### 注意的点

- ALU的计算在本次C++模拟中均为**组合逻辑**，不占用单独的clock。
- ALU的运行有**单独**的Execute模块，将RS中的值直接传递给ALU（**组合逻辑**），然后算出结果。
- Issue和Execute分开，普通指令的Issue就是读取指令，解析进入RS；Load 和 Store指令可能解析后还要发送给LSB（**时序逻辑**），LSB自带一个地址计算单元（**组合逻辑**），一旦接受到数据就可以立马得到目标内存地址。
- CDB总线采用**时序逻辑**。
- 目前打算只实现一个ALU计算单元，如果有很多个ALU计算单元，每一个ALU计算单元会有单独的保留站，如果同时有多个ALU单元计算完成，则只能选择一个进行广播（CDB）或者增加CDB的带宽。
- Store 和 Load：全部由LSB来Memory Access。ROB 在 commit 时发信息给Store Buffer，Store Buffer下一个周期再Memory Access；Issue 完之后发送给LSB，然后下一个周期 Load Buffer再check队首看可不可以Memory Access。

### 函数的设计

- issue()
- execute()
- memory_access()
- write_CDB()
- commit()
