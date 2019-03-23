#树梅派的启动过程

树莓派的启动过程分为四个阶段

- First stage bootloader
树莓派上电后，secure core执行SoC(BCM2837B0)中的bootloader，其作用是挂载SD卡上的FAT32分区，从而加载下一阶段的bootloader。这个部分的程序在SoC的ROM中，用户无法修改。

- Second stage bootloader
该阶段在SD卡的FAT32分区中寻找bootcode.bin，该文件相当于GPU的引导文件，GPU将bootcode.bin读取到二级缓存，开始执行bootcode.bin，该程序会从SD卡上检索GPU固件(start.elf)，从而启动GPU。

- GPU firmware(start.elf)
start.elf被加载之后就可以使GPU启动CPU。start.elf会让GPU读取系统配置文件config.txt，之后会读取cmdline.txt，该文件中包含内核运行的参数，用来初始化CPU的clock和串口等设备，然后将kernel7.img加载到内存中，最后触发CPU的reset，启动CPU。

- User code(kernel7.img)
该阶段中CPU启动后开始执行kernel7.img中的指令，初始化操作系统内核。

2000年之前主流计算机的启动均是BIOS引导，现在逐渐被UEFI取代。下面简述BIOS的启动流程。

- 上电
按下电源键之后，会启动CPU，随即选取一个CPU作为启动CPU，运行BIOS内部程序，其余CPU停机直到操作系统开始使用他们，此时CPU处于实模式。

- 重置向量
CPU启动后指令寄存器(EIP)被初始化，此时CPU会对EIP的初始值加上一个基址寄存器的值，生成一个32位的地址0xFFFFFFF0。0xFFFFFFF0也被称为重置向量(reset vector)，该地址处是一条JUMP指令，使指令寄存器跳到BIOS开始处(0xF0000)。

- BIOS初始化与BIOS POST(加电自检)
0xF0000地址实际上是BIOS中的boot block的开始处。在这个阶段，会初始化部分硬件。
初始化完成后，CPU进行BIOS加电自检(power on self test, POST)。

- BIOS记录系统设定值
检查完成后BIOS会确定计算机有哪些资源，会在内存中设置一系列数据来描述硬件信息。

- 搜索MBR
到这一步，BIOS开始尝试加载操作系统。假设在硬盘上找到了操作系统，它会首先读取硬盘上的大小为512B的0号扇区，这个扇区被称为主引导记录(MBR)，BIOS读完磁盘上的MBR之后会把它拷贝到内存0x7C00地址处，然后CPU跳转到该内存地址执行MBR里的指令。之后引导操作系统。

和主流计算机相比，树莓派上没有BIOS，大部分系统配置的文件都在SD卡上。

树莓派启动时用到了FAT32文件系统，因为第一阶段的bootloader就挂载了SD卡上的FAT32文件系统。