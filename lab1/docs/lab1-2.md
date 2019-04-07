# lab1-2实验报告

#### 裁剪内核

裁剪内核的目标为：在`init`能正常运行的前提下关闭编译选项，使编译出的`.img`文件尽量小。

判断裁剪是否成功的标准是：树莓派上电后LED灯是否正常闪烁。（注：由于没有外接屏幕，判断标准中没有“在屏幕上打印 Hello OSH-2019”，因此我将与外接显示器有关的选项都关闭了）

最终裁剪得到的`kernel7.img`文件大小约为1.6MB。下面罗列并说明我关闭的选项。（注：以缩进表示子目录与父目录的关系）

- general setup  ---> 
	- support for paging of anonymous memory(swap)
	该选项为是否使用SWAP虚拟内存管理。由于本次实验仅需要让LED闪烁，需要的内存空间很小，因此我认为不需要SWAP虚拟内存管理。
	
	- IRQ subsystem  ---> 
		- Expose irq internals in debugfs
	该选项为通过debugfs公开内部状态信息。 主要用于开发人员和调试难以诊断的中断问题，本次实验不需要。

	- timer subsystem  ---> 
		- old idle dynticks config
 	该选项是为了兼容以前的配置文件而存在的，不需要选中
 
		- High-Resolution Timer
	该选项作用是在纳秒级别处理内部时间分片。	若是禁用High-Resolution Timer，那么内核是在毫秒级别处理内部时间分片。

	- cpu/task time and stats accouting  ---> 
	该目录下的选项与CPU/进程的时间以及状态统计有关，均不需要。

	- Kernel .config support
	作用是把内核的配置信息编译进内核中,以后可以通过scripts/extract-ikconfig脚本从内核镜像中提取这些信息，本次实验不需要从编译好的内核镜像中提取配置信息。

	- Control Group support  ---> 
	Cgroup(Control Group)是一种进程管理机制,可以针对一组进程进行系统资源的分配和管理，本次实验进程很少，因此把该选项关闭。

	- namespaces support  ---> 
	命名空间支持.主要用于支持基于容器的轻量级虚拟化技术(比如LXC和Linux-VServer以及Docker)。

	- automatic process group scheduling
	该选项通过自动创建和控制任务分组，为普通的桌面应用场景进行调度优化。通过任务分组可以将那些占用CPU过高的任务与普通的桌面应用程序分离开来。没有桌面环境，因此不需要。

	- support initial ramdisks compressed using ***
	指定initramfs映像使用的压缩算法，支持gzip/bzip2/lzma/xz/lz0/lz4等。只留下了gzip，其他的都去掉了，因为解压算法是gzip。

	- Compiler optimization level
	选择了`optimize for size`，在编译内核时将传递-Os优化选项给编译器,可以优先服务大小，使编译出的内核尽量小。
   
	- Configure standard kernel features (expert users)  ---> 
		
		- Sysfs syscall support 
	是否开启已被反对使用的sys_sysfs系统调用(已不再被libc支持)，关闭。
   
	- Use full shmem filesystem
	完全使用shmem来代替ramfs.shmem是基于共享内存的文件系统(可以使用swap)，它比简单的ramfs先进许多。仅在微型嵌入式环境中且没有swap的情况下才可能会需要使用原始的ramfs。本次实验不需要使用shmem，因此关闭。

	- enable aio support
	可能用在某些多线程的程序中。
    
	- Enable madvise/fadvise syscalls
	在没有用到系统调用的嵌入式设备中可以取消该选项。
    
	- Kernel Performance Events And Counters  ---> 
	与提升系统性能相关，不必要

	-enable vm event counter for /proc/vmstat
	为/proc/vmstat提供额外的虚拟内存子系统事件统计信息，不必要。

	- enable slub debugging support
	提供slub调试支持，以及/sys/slab接口，不需要调试，不必要。
    
	- profiling support
	剖面支持，用一个工具来扫描和提供计算机的剖面图。支持系统评测，性能分析，不必要。

	- kprobes
	Kprobe机制是内核提供的一种调试机制，不必要。


	- Enable loadable module support ---> 
 	里面的所有选项均可以删除，但是需要选中该目录，否则会造成无法编译。

- Enable the block layer
块设备支持，如硬盘、U盘等，该目录下下面几个选项可以裁剪，因为我们只需要用到SD卡。
	- Support for large (2TB+) block devices and files
	- Block layer bio throttling support
	- Block layer debugging information in debugfs
	- Block layer SG support v4 helper lib
 	- IO Scheduler---> 

- Kernel Features  --->

- CPU Power Management  --->
子选项全部裁剪，该选项有好处但不必要。

- Floating point emulation  --->
只保留了第一项，因为我们用不到复杂的浮点运算。

- Userspace binary formats  --->
只保留了前两项，`Kernel support for ELF binaries`和`Kernel support for scripts starting with #!`，其他的均为运行高级语言如Java和调试bug所需要的选项，可以删除。

- Power management options  ---> 
全部裁剪，选中会有好处，但我的目的是使内核尽量小，所以删除。

- Networking support  ----
本次实验不需要网络，所以全部裁剪

- Device Drivers  --->
该目录中内容较多，可以裁剪的选项也比较多。
	
	- Generic Driver Options  ---> 

		- Support for uevent helper
	早年的内核(切换到基于netlink机制之前),在发生uevent事件(通常是热插拔)时，需要调用用户空间程序，以帮助完成uevent事件的处理。此选项就是用于开启此功能.由于目前的发行版都已不再需要此帮助程序,所以裁剪了。

	- Memory Technology Device (MTD) support  ---->
	MTD子系统是一个闪存转换层.其主要目的是提供一个介于闪存硬件驱动程序与高级应用程序之间的抽象层,以简化闪存设备的驱动，本实验不需要。
	
	- Block devices  ---->
	所有选项均裁剪，本次实验只用到了SD卡。

	- SCSI device support  ---->
	在该目录中把tape、CDROM和其他特殊设备的支持去掉了，选项如下。
		- SCSI tape support
		- SCSI OnStream SC-x0 tape support
		- SCSI CDROM support
		- SCSI generic support
	
	- Multiple devices driver support (RAID and LVM)
	可以全部裁剪，在本次实验中不必要。
	
	- input device support 
	通用输入层，里面除了自动选中的选项外全部裁剪了，因为本实验不需要用到诸如鼠标键盘操纵杆之类的设备。

	- Character devices  --->
	本实验不需要使用终端，所以我裁剪了里面大部分内容，留下了和Broadcom平台相关的选项。
	
	- Voltage and Current Regulator Support  --->
	通用的电压与电流调节器框架.除了提供通用的电压与电流调节接口外,还能通过sysfs向用户空间提供电压与电流的状态信息。有好处，但是对于本次实验不必要。
	
	- Remote Controller support  --->
	远程控制支持，不必要。
	
	- Multimedia support  ----
	各种多媒体设备的支持，不需要。
	
	- Generic Thermal sysfs driver  ----为ACPI规范中定义的"thermal"(发热控制)提供一个通用的sysfs接口,以方便与诸如温度传感器和风扇之类的设备通信，本实验不需要。
	
	- Graphics support  ---> 
	本次实验未用到图形界面，因此可以裁剪。
	
	- Sound card support  ----
	声卡支持也不需要。
	
	- HID support  ---> 
	HID(人机接口设备)是一种定义计算机如何与人类交互的规范,常与USB或蓝牙搭配使用,常见的设备有:键盘,鼠标,触摸板等等，本实验并不需要。
	
	- Real Time Clock  ----
	通用RTC(实时时钟)类支持，不需要。
	
- File systems  --->
除了ext4相关选项和均可以裁剪，因为kernel.img中挂载了ext4分区。

- Kernel hacking  ---> 
kernel hacking与内核调试相关，本次实验无需调试内核，因此删除。

- Security options
与安全有关的选项全部不需要，因此全部裁剪。

- Cryptographic API
提供核心的加密API支持，这里的加密算法内核中若有其他部分依赖它，会自动选上，其他可以全不选。

- Library routines  --->
例行库，内核中若有其他部分依赖它，会自动选上，其他选项可以全部删除。

#### 思考题

1.`/dev/sdc1` 中除了 `kernel7.img` 以外的文件哪些是重要的？他们的作用是什么？

`bootcode.bin`该文件在启动阶段被用到，该程序会从SD卡上检索GPU固件(start.elf)，从而启动GPU。`start.elf`会让GPU读取系统配置文件`config.txt`，之后会读取`cmdline.txt`，该文件中包含内核运行的参数，用来初始化CPU的clock和串口等设备，然后将`kernel7.img`加载到内存中，最后触发CPU的reset，启动CPU。以上提到的文件均是重要的。

2.`/dev/sdc1`中用到了什么文件系统，为什么？可以换成其他的吗？

片上ROM里的出厂代码决定了：GPU启动之后，只能去SD卡上的FAT32格式的文件系统里的目录下，寻找一个叫`bootcode.bin`的启动文件。所以，将SD卡里肯定有一个FAT32的分区。不可以换成其他的。

3./dev/sdc1 中的 kernel 启动之后为什么会加载 /dev/sdc2 中的 init 程序？

内核文件(kernel7.img)加载以后，就开始运行第一个程序`init`，它的作用是初始化系统环境。运行`init`使Linux启动必不可少的过程。