# If you want to create your own OS container, use this file as a template or as a base
FROM fedora:38

LABEL author=j1sk1ss
LABEL os=cordellOS

WORKDIR /home

RUN dnf -y update 
RUN dnf -y install git nano vim sudo gcc gcc-c++ make bison flex gmp-devel libmpc-devel mpfr-devel texinfo wget \
                   nasm mtools python3 python3-pip python3-pyparted python3-scons dosfstools guestfs-tools qemu-system-x86 grub-customizer
RUN mkdir /home/os-dev
RUN mkdir /home/os-dev/project
RUN mkdir /home/os-dev/tool_chain
RUN mknod /dev/loop-control c 10 237 || true
RUN for i in $(seq 0 7); do mknod /dev/loop$i b 7 $i || true; done

COPY tool_chain /home/os-dev/tool_chain/
