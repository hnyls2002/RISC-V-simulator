# RISC-V-simulator

Tomasulo

### 时序逻辑 & 组合逻辑

- 组合逻辑任意时刻的输出仅仅取决该时刻的输入，与时钟无关。
- 时序逻辑先得到了输入的结果，在下一个周期的上升沿才会把结果更新到部件，此处的结果是上个周期另一个部件的输出。
- 组合逻辑和时序逻辑的结合：例如ALU设计成组合逻辑，那可以认为ALU**一直接受**输入信号，**近似于瞬间**的计算后，一直发送给CDB，但鉴于所有的存储部件都是时序逻辑，实际上ALU也是在**一个周期内将RS中的值传递给CDB**。
- 例如ALU、Issue这样**单独的部件**，组合逻辑可以单独写出来（模拟），时序逻辑部件中的组合逻辑电路没有必要单独写出来。

### Hazards Solution

所有的write指令全部被rename

- WAW hazard：两个write操作，rd相同的情况下，由于指令是顺序issue的，第二个指令会issue的时候会覆盖掉register status的装填，标记为第二个指令的rename结果，于是第一个指令就算后执行完，也并不会和register status匹配。
- WAR hazard：先read后write，由于Reservation Station中直接存储了register的value（an immediate），所以之后再与从哪个register中读取数据无关了，第二个write操作想怎么弄，就怎么弄。
- RAW hazard：先write后read，第一条指令issue之后rename为Reservation Station的编号，于是第二条指令issue的时候检查Register Status，发现这个寄存器被rename了，于是真正的依赖关系产生了。

### Load & Store

- Load 和 Store 要求从内存中读取数据，内存和寄存器一样，也是存储数据的，所以也会有hazard出现。
- 对于Load 和 Store操作，建立Load Store Buffer，相当于普通操作的Reservation Station。

### 注意的点

- Decode、ALU、Commit的部分采用**组合逻辑**（不存储数据，相当于只是计算单元）
- CDB总线采用**时序逻辑**。
- 目前打算只实现**一个ALU计算单元**，如果有很多个ALU计算单元，每一个ALU计算单元会有单独的保留站，如果同时有多个ALU单元计算完成，则只能选择一个进行广播（CDB）或者增加CDB的带宽。
- **Store 和 Load** ：**全部由LSB来Memory Access**。ROB 在 commit 时发信息给Store Buffer，Store Buffer下一个周期再Memory Access；Issue 完之后发送给LSB，然后下一个周期 Load Buffer再check队首看可不可以Memory Access。
- Memory Access 用三个周期来实现。

### 组合逻辑之间的串联关系

- Issue 一条指令的时候，先到RegFile里面找，没有找到再到ROB里面找，发现也没有。但如果**恰好是这个周期Execute完成**，而Issue是组合逻辑，指令还并没有储存到RS中，Execute的WriteBack自然也在RS中找不到值。下一个周期开始虽然ROB中更新了，但我们约定**一条指令的Ready状态更新只在Execute完之后的WriteBack阶段**，所以这条指令就再也不会Ready了。
- Load 的MemoryAccess部分同理，注意MemoryAccess分成三个周期（也是组合逻辑）

### 函数的设计

- **I**nstruction **Q**ueue
- **R**eservation **S**tation
- **L**oad & **S**tore **B**uffer
- **R**egister **F**ile
- **R**e**O**rder **B**uffer
- **C**ommon **D**ata **B**us
- **U**pdate ：更新静态的部件（Register，Memory...）
- **ALU**
- **I**ssue ：解码电路
- **C**ommit ：提交时的判断电路
