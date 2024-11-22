# CordellOS.MPRJ
Simple OS based on .asm / .c double stage boot loader, and .c / .cpp based kernel with GCC .c / .cpp compiler. 
</br>
For building you can download image in packages, then run container with next commands:</br>

    docker run -it -v [path to your project]:/home/os-dev/project --privileged=true --rm ghcr.io/j1sk1ss/os-dev-env:v02

                    
Where:</br>
-it - means that when you run this container you will get command line (fedora 38)</br>
-v [path]:[path] - means that we connect your project with container directory</br>
--privileged=true - needs for working with dev loop (Optional for creating images of OS)</br>
--rm - delete container after use (Optional)</br>
</br></br>
Before compile be sure that your script has right link to tool_chain. In this container tool_chain placed in 

    /home/os-dev/tool_chain

Used GSS 11.2.0</br>
And used binutils 2.37</br>

Also this container contains fedora 38, gcc, gcc-c++, make, bison, flex, gmp-devel, libmpc-devel, mpfr-devel, texinfo, nasm, mtools, qemu-system-x86, and grub tools.</br>
This container tested in MacOS 10.15, Windows 10 (On win10 can be problems with image building. But compiling works fine) and Fedora 38</br> 
    
</br>

| TODO List:                             |Status:  | Features:                                                     |
|----------------------------------------|---------|---------------------------------------------------------------|
| [?] - WIP                              |         |                                                               |
| [&check;] - complete                   |         |                                                               |
| **1) Multiboot struct**                |[&check;]|   1) Physical memory manager                                  |
| **2) Phys and Virt manages**           |[&check;]|   2) Virtual memory manager (paging)                          |
|    2.0) Make page and phys prints...   |[&check;]|   3) Multitasking                                             |
| **3) ELF check v_addr**                |[&check;]|   4) Syscalls                                                 |
|    3.1) Fix global and static vars     |[&check;]|       4.0) Std libs for user programs                         |
|    3.2) Loading ELF without malloc...  |[&check;]|       4.1) Cordell-ASM compiler                               |
| **4) Paging (create error with...**    |[&check;]|   5) Mouse & keyboard support                                 |
|    4.0) Random Page fault (Null...     |[&check;]|   6) VBE support                                              |
|    4.1) Tasking with paging            |[&check;]|   7) ELF support                                              |
|    4.2) ELF exec with tasking...       |[&check;]|   8) BMP support                                              |
| **5) VBE / VESA**                      |[&check;]|   9) FAT32/16/12 support                                      |
|    5.0) VBE kernel                     |[&check;]|   10) Boot config                                             |
|        5.0.1) Kshell scrolling         |[ ]      |   11) Multidisk support                                       |
|    5.1) Double buffering               |[ ]      |   12) Networking                                              |
| **6) Keyboard to int**                 |[&check;]|       12.0) DHCP                                              |
| **7) Reboot outportb(0x64, 0xFE);**    |[&check;]|       12.1) UDP                                               |
| **8) Mouse support**                   |[&check;]|       12.2) ARP                                               |
|    8.0) Std lib for graphics           |[&check;]|       12.3) RLT8139 driver                                    |
|        8.0.0) Objects                  |[ ]      |                                                               |
|        8.0.1) Click event              |[ ]      |                                                               |
|    8.1) Loading BMP without malloc...  |[&check;]|                                                               |
|    8.1) Syscalls to std libs           |[&check;]|                                                               |
|        8.1.0) Syscalls for content...  |[&check;]|                                                               |
|        8.1.1) Syscalls for content...  |[&check;]|                                                               |
|        8.1.2) Syscalls for _kmallocp... |[&check;]|                                                               |
|    8.2) VBE userland                   |[ ]      |                                                               |
|        8.2.0) VBE file manager         |[?]      |                                                               |
|        8.2.1) VBE text editor          |[?]      |                                                               |
| **9) Malloc optimization**             |[ ]      |                                                               |
| **10) Bugs**                           |[?]      |                                                               |
|    10.0) Tasking page fault...         |[?]      |                                                               |
|    10.1) Mouse page fault              |[&check;]|                                                               |
|    10.2) Tasking with page allocator   |[&check;]|                                                               |
| **11) Ethernet**                       |[?]      |                                                               |
|    11.0) ethernet                      |[?]      |                                                               |
|        11.0.0) Send packets            |[&check;]|                                                               |
|        11.0.1) Receive packets         |[&check;]|                                                               |
|        11.0.2) DHCP                    |[&check;]|                                                               |
|        11.0.3) Sending data            |[&check;]|                                                               |
|            11.0.3.0) Sending normal... |[&check;]|                                                               |
|    11.1) IP Udp, Arp                   |[&check;]|                                                               |
|    11.2) Receive data on Host server   |[&check;]|                                                               |
|    11.3) STD libs for networking       |[&check;]|                                                               |
|    11.4) TCP                           |[ ]      |                                                               |
|    11.5) Host sending to VK/TG etc     |[ ]      |                                                               |
| **12) KShell to ELF program**          |[&check;]|                                                               |
|    12.1) MEM-DATA progs                |[&check;]|                                                               |
| **13) DOOM?**                          |[ ]      |                                                               |



![Alt Text](https://github.com/j1sk1ss/CordellOS.MPRJ/blob/main/cover.png)
