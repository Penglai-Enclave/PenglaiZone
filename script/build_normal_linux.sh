if test -z "$SECURE_WORLD_DIR";then
	echo "SECURE_WORLD_DIR is null string"
    exit 1
else
	echo $SECURE_WORLD_DIR
fi

# get hart number
CPU_NUM=$(cat /proc/cpuinfo| grep "processor"| wc -l)

mkdir -p $SECURE_WORLD_DIR/out/opensbi
mkdir -p $SECURE_WORLD_DIR/out/uboot
mkdir -p $SECURE_WORLD_DIR/out/normal_linux/kernel
mkdir -p $SECURE_WORLD_DIR/out/normal_linux/driver
mkdir -p $SECURE_WORLD_DIR/out/normal_linux/sdk
mkdir -p $SECURE_WORLD_DIR/out/secure_linux

# build normal linux
cd $SECURE_WORLD_DIR/starfive-normal-linux
# git checkout f81e6caa4728ff4fab86112a1c45bf031cd8f7d3
# cp $SECURE_WORLD_DIR/package/brcmfmac43430-sdio.bin firmware/brcm/
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv visionfive_defconfig
# make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv dtbs
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv -j$CPU_NUM
mv arch/riscv/boot/Image.gz $SECURE_WORLD_DIR/out/normal_linux/kernel/fedora_with_modules.gz
mv arch/riscv/boot/dts/starfive/jh7100-starfive-visionfive-v1.dtb $SECURE_WORLD_DIR/out/normal_linux/kernel/visionfive-v1-uart-reserved-plic-nonet.dtb

if true; then
# build user-level sdk and driver for secure-linux installation
cd $SECURE_WORLD_DIR/starfive-penglai
cd penglai-selinux-driver && make && cp penglai_linux.ko $SECURE_WORLD_DIR/out/normal_linux/driver/ && cd ..
cd penglai-selinux-sdk && make && cp host/host $SECURE_WORLD_DIR/out/normal_linux/sdk/

fi
