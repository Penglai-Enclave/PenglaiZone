# Penglai-secure-world

## RISC-V TEE 安全/非安全世界模型

实现安全/非安全世界间内存隔离，为安全世界提供设备安全能力。在安全世界中运行Linux，从而实现对现有成熟Linux程序的后向兼容。

![alt arch.png](images/arch.png)

## 仓库结构

需要编译的 Artifacts 包括 Visionfive V1 开发板上除 secondBoot 和 ddrinit 外的所有软件，下图显示了主代码仓库Penglai-secure-world的目录结构，接下来将根据启动顺序依次介绍代码仓库和编译对象间的对应关系：

```
➜  Penglai-secure-world git:(main) ✗ tree
.
├── starfive-normal-linux
├── starfive-penglai
│   ├── conf
│   ├── docs
│   ├── openEuler-files
│   ├── openeuler-kernel
│   ├── opensbi-0.9
│   ├── patches
│   ├── penglai-enclave-driver
│   ├── penglai-selinux-driver
│   ├── penglai-selinux-sdk
│   ├── riscv-qemu
│   ├── scripts
│   └── sdk
├── starfive-secure-linux
│   ├── bsp
│   ├── buildroot
│   ├── conf
│   ├── HiFive_U-Boot
│   ├── linux
│   ├── opensbi
│   └── soft_3rdpart
└── starfive-uboot
```
- secondBoot 和 ddrinit 直接使用赛昉预编译好的可执行文件，文件下载以及烧录、重置等相关操作请参考[VisionFive Single Board Computer Quick Start Guide.pdf](https://starfivetech.com/uploads/VisionFive%20Single%20Board%20Computer%20Quick%20Start%20Guide.pdf)
- OpenSBI & u-boot 是实施了 domain 机制，添加了蓬莱 Secure monitor 的，分别对应子仓库starfive-penglai/opensbi-0.9 和 starfive-uboot，从源码编译请参考[VisionFive Single Board Computer Software Technical Reference Manual.pdf](https://starfivetech.com/uploads/VisionFive%20Single%20Board%20Computer%20Software%20Technical%20Reference%20Manual.pdf)，其编译产物为 firmware_payload.bin.out。
- Normal Linux 所需的内核镜像、设备树由子仓库starfive-normal-linux 编译得到，仍参考[VisionFive Single Board Computer Software Technical Reference Manual.pdf](https://starfivetech.com/uploads/VisionFive%20Single%20Board%20Computer%20Software%20Technical%20Reference%20Manual.pdf)，编译产物为fedora_with_modules.gz和visionfive-v1-uart-reserved-plic-nonet.dtb。
- Normal Linux 用于加载 Secure Linux 的内核驱动和用户态程序将从子仓库starfive-penglai编译，分别对应penglai-selinux-driver和penglai-selinux-sdk这两个目录，编译产物为penglai_linux.ko 和host。
- Secure Linux 对应子仓库starfive-secure-linux，其内核镜像和设备树从目录starfive-secure-linux/linux编译，其 initramfs 从目录starfive-secure-linux/buildroot编译并打包在内核镜像中，编译产物为sec-image 和 sec-dtb.dtb。

## 运行测试

介绍在赛昉VisionFive v1开发板上的部署并测试的过程。

### 一、工具与环境准备

#### 硬件
如下图，硬件需要两个串口转换器，分别连接板上的UART3和UART1，参考[StarFive 40-Pin GPIO Header User Guide](https://doc-en.rvspace.org/General/PDF/StarFive_40-Pin_GPIO_Header_UG.pdf)完成接线，同时还需要一张SD卡和一个SD卡读卡器。主机软件可使用minicom 来连接串口。

![alt hardware.png](images/hardware.png)

#### 软件环境
使用 Ubuntu 20.04.5 LTS x86_64 作为编译和制作SD卡的软件环境，如图所示：

![alt hardware.png](images/software_env.png)

### 二、代码仓库初始化

克隆代码仓库，更新子模块，并安装编译所需依赖。项目编译后约20G，请留意磁盘空间。
```
git clone https://github.com/Shang-QY/Penglai-secure-world.git
cd Penglai-secure-world
git submodule update --init --recursive
```
```
sudo apt update
sudo apt upgrade
sudo apt-get install autoconf automake autotools-dev bc bison xxd \
build-essential curl flex gawk gdisk git gperf libgmp-dev \
libmpc-dev libmpfr-dev libncurses-dev libssl-dev libtool \
patchutils python screen texinfo unzip zlib1g-dev device-tree-compiler libyaml-dev
```
设置后续操作中要使用的、代表仓库根目录的环境变量。
```
export SECURE_WORLD_DIR=$(pwd)
```

### 三、编译

一键编译所有软件模块，编译所需的时间较长。
```
./script/build_all.sh
```
在编译结束后，最终需要的编译产物被放在`out`目录下，结构如下所示：

![alt hardware.png](images/compile_result.png)

### 四、制作SD卡

一键制作SD卡，将插有SD卡的读卡器连接到主机（Linux系统），确定SD卡对应的设备，图中SD卡对应的设备为`/dev/sda`：

![alt hardware.png](images/sdcard.png)

执行脚本并以设备名为参数（执行过程较快，起初需要输入sudo密码）：
```
./script/make_sdcard.sh -d /dev/sda
```

### 五、安装软件到SD卡中

一键安装编译产物到SD卡中（除`fw_payload.bin.out`外），此过程非常快。
```
./script/install_all.sh -d /dev/sda
```

### 六、烧录固件

请参考[VisionFive Single Board Computer Quick Start Guide.pdf](https://starfivetech.com/uploads/VisionFive%20Single%20Board%20Computer%20Quick%20Start%20Guide.pdf)中的`Appendix B: Updating Firmware and u-boot`部分，烧录fw_payload.bin.out

### 七、启动运行

使用minicom分别连接好两个串口。根据赛昉引脚手册[StarFive 40-Pin GPIO Header User Guide](https://doc-en.rvspace.org/General/PDF/StarFive_40-Pin_GPIO_Header_UG.pdf)，可以看到UART3是ddrinit、secondboot、OpenSBI、U-boot所使用的串口，即系统启动早期输出将从UART3打印。之后Normal linux使用UART1作为串口设备启动，启动过程将将从UART1打印。后续被加载并启动的Secure linux则使用UART3作为串口设备。

上电，在UART1上看到Normal Linux 的启动过程，（使用用户名：riscv，密码：starfive）进入终端。执行：
```
sudo ./run.sh
```
之后将在UART3上看到Secure Linux 的启动过程，（使用用户名：root，密码：starfive）进入终端。接着在Secure Linux 的终端执行`python`即可进入python 命令行。

运行结果截屏如图所示：（左边是UART3串口-安全世界终端，右边是UART1串口-非安全世界终端）

![alt hardware.png](images/screen_cap.png)
