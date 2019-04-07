# lab1-3实验报告

led闪烁的汇编代码如下

~~~
.section .init
.global _start
_start:

ldr r0, =0x3F200000		@ GPIO对应的内存地址
ldr r2, =0x3F003000		@ 计时器控制器的内存地址

loop:
	ldr r3, [r2, #4]   	@ 读取计时器的低 4 字节
	lsl r3, #11			@ 将低四字节左移11位（2^21 = 209 7152）
	lsr r3, #11			@ 再右移11位，即为按位与
	
	mov r1, #1
	lsl r1, #16			@ 2^16 = 65536,约为0.06s

	cmp r3, r1  		@ 与2 * 10^6比较
	bls open			@ 若小于等于则跳转,打开LED
	
	mov r1, #0			@ 将GPIO29(ACT LED)设置为了非输出模式
	str r1, [r0, #8]
	
	str r1, [r0, #28]	@ 将GPIO29(ACT LED)关闭

	mov r1, #1			@ 将GPIO29关闭，熄灭LED
	lsl r1, #29
	str r1, [r0, #40]
b loop

open:
	mov r1, #1			@ 将GPIO29(ACT LED)设置为了输出模式
	lsl r1, #27
	str r1, [r0, #8]
	
	lsl r1, #2			@ 将GPIO29(ACT LED)打开，即点亮LED
	str r1, [r0, #28]

	mov r1, #0			@ 将GPIO29的关闭取消
	str r1, [r0, #40]
b loop

~~~

使用计数器的低21位，因为2^21^ = 2097152比较接近2 * 10^6^，为2s，在前65536个周期(约为0.06s)打开led灯，其余时间关闭，即可实现闪烁。

### 思考题 

1.在这一部分实验中 `/dev/sdc2` 是否被用到？为什么？

`/dev/sdc2`未被用到，由树梅派的启动过程(lab1-1.md)可知，树莓派启动的四个阶段只用到了FAT32`/dev/sdc1`，第四步为CPU开始执行`kernel7.img`中的指令。lab1中用到了`/dev/sdc2`，是因为`kernel7.img`中的指令挂载了该分区。

但在本实验中`kernel7.img`的作用仅仅是使LED灯闪烁，并没有挂载`/dev/sdc2`，所以本实验中未用到`/dev/sdc2`分区。

2.生成 `led.img` 的过程中用到了 `as`,`ld`, `objcopy` 这三个工具，他们分别有什么作用，我们平时编译程序会用到其中的哪些？

- `as`是汇编器，汇编器是将汇编语言翻译为机器语言的程序。一般而言，汇编生成的是目标代码，需要经链接器生成可执行代码才可以执行。

- `ld`是链接器 负责将多个*.o的目标文件链接成elf(Executable and Linkable Format )可执行文件。elf可执行文件是unix常用的可执行文件类型，就像windows的exe文件。

- `objcopy`可以将目标文件的一部分或者全部内容拷贝到另外一个目标文件中，或者实现目标文件的格式转换。本实验中使用的功能是文件格式转换。
 elf文件中有很多信息包括段信息还有头信息，这些信息对硬件是没有意义的，所以有的时候我们通过`objcopy`将elf转化成img文件加载到内存中运行，img文件就是一个纯二进制文件。



