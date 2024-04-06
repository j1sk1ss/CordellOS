# We use Fedora 38 image
FROM fedora:38

# Info labels
LABEL author=j1sk1ss
LABEL os=cordellOS

# Set workdir
WORKDIR /home

# Install all dependencies
RUN dnf -y update 
RUN dnf -y install git nano vim gcc gcc-c++ make bison flex gmp-devel libmpc-devel mpfr-devel texinfo wget \
                   nasm mtools python3 python3-pip python3-pyparted python3-scons dosfstools guestfs-tools qemu-system-x86 grub-customizer
RUN mkdir /home/os-dev
RUN mkdir /home/os-dev/project
RUN mkdir /home/os-dev/tool_chain
RUN mknod /dev/loop0 b 7 0

# Copy toolchain
COPY tool_chain /home/os-dev/tool_chain/