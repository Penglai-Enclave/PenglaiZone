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

# build fw_payload.bin.out
cd $SECURE_WORLD_DIR/starfive-uboot
make starfive_jh7100_visionfive_smode_defconfig ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-
make u-boot.bin u-boot.dtb ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-
cd $SECURE_WORLD_DIR/starfive-penglai/opensbi-0.9
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- PLATFORM=generic FW_PAYLOAD_PATH=$SECURE_WORLD_DIR/starfive-uboot/u-boot.bin FW_FDT_PATH=$SECURE_WORLD_DIR/starfive-uboot/u-boot.dtb
cd $SECURE_WORLD_DIR
./script/fsz.sh starfive-penglai/opensbi-0.9/build/platform/generic/firmware/fw_payload.bin fw_payload.bin.out
mv starfive-penglai/opensbi-0.9/build/platform/generic/firmware/fw_payload.bin.out out/opensbi/

# build normal linux
cd $SECURE_WORLD_DIR/starfive-normal-linux
# git checkout f81e6caa4728ff4fab86112a1c45bf031cd8f7d3
# cp $SECURE_WORLD_DIR/package/brcmfmac43430-sdio.bin firmware/brcm/
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv visionfive_defconfig
# make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv dtbs
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv -j$CPU_NUM
mv arch/riscv/boot/Image.gz $SECURE_WORLD_DIR/out/normal_linux/kernel/fedora_with_modules.gz
mv arch/riscv/boot/dts/starfive/jh7100-starfive-visionfive-v1.dtb $SECURE_WORLD_DIR/out/normal_linux/kernel/visionfive-v1-uart-reserved-plic-nonet.dtb

# build user-level sdk and driver for secure-linux installation
cd $SECURE_WORLD_DIR/starfive-penglai
cd penglai-selinux-driver && make && cp penglai_linux.ko $SECURE_WORLD_DIR/out/normal_linux/driver/ && cd ..
cd penglai-selinux-sdk && make && cp host/host $SECURE_WORLD_DIR/out/normal_linux/sdk/

# build secure linux
cd $SECURE_WORLD_DIR/starfive-secure-linux
git checkout origin/origin_JH7100_VisionFive
cd buildroot && git checkout JH7100_VisionFive_devel && cd ..
cd HiFive_U-Boot && git checkout JH7100_VisionFive_devel && cd ..
cd linux && git checkout origin/visionfive-5.15.y-devel && cd ..
cd opensbi && git checkout master && cd ..
make -j$CPU_NUM
make clean          # Now we use this special compile method, refine in the future
git checkout main
cd linux && git checkout main && cd ..
make -j$CPU_NUM
cp work/linux/arch/riscv/boot/Image $SECURE_WORLD_DIR/out/secure_linux/sec-image
cp work/linux/arch/riscv/boot/dts/starfive/jh7100-starfive-visionfive-v1.dtb $SECURE_WORLD_DIR/out/secure_linux/sec-dtb.dtb

# generate grub.cfg and run.sh
cat > $SECURE_WORLD_DIR/out/uboot/grub.cfg <<EOF
set default=0
set timeout_style=menu
set timeout=2

set debug="linux,loader,mm"
set term="vt100"

menuentry 'boot_by_UART1 plic_hart0 no_network Fedora_with_modules reserved_payload linux-5.15.10 visionfive' {
    linux /fedora_with_modules.gz ro root=UUID=59fcd098-2f22-441a-ba45-4f7185baf23f rhgb console=tty0 console=ttyS2,115200 earlycon rootwait stmmaceth=chain_mode:1 selinux=0 LANG=en_US.UTF-8
    devicetree /dtbs/5.15.10+/starfive/visionfive-v1-uart-reserved-plic-nonet.dtb
        initrd /initramfs-5.15.10+.img
}
EOF

cat > $SECURE_WORLD_DIR/out/normal_linux/sdk/run.sh <<EOF
cd new_sec
sudo insmod penglai_linux.ko
./host sec-image 0xc0200000 sec-dtb.dtb 0x186000000
EOF
