.section .init
.global _start
_start:

ldr r0, =0x3F200000		@ GPIO对应的内存地址
ldr r2, =0x3F003000

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
	
