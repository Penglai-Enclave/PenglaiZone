if test -z "$SECURE_WORLD_DIR";then
	echo "SECURE_WORLD_DIR is null string"
else
	echo $SECURE_WORLD_DIR
fi

# get hart number
CPU_NUM=$(cat /proc/cpuinfo| grep "processor"| wc -l)

# build fw_payload.bin.out
cd $SECURE_WORLD_DIR/starfive-uboot
make starfive_jh7100_visionfive_smode_defconfig ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-
make u-boot.bin u-boot.dtb ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-
cd $SECURE_WORLD_DIR/starfive-penglai/opensbi-0.9
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- PLATFORM=generic FW_PAYLOAD_PATH=$SECURE_WORLD_DIR/starfive-uboot/u-boot.bin FW_FDT_PATH=$SECURE_WORLD_DIR/starfive-uboot/u-boot.dtb
cd $SECURE_WORLD_DIR
./script/fsz.sh starfive-penglai/opensbi-0.9build/platform/generic/firmware/fw_payload.bin out/fw_payload.bin.out

# build normal linux
cd $SECURE_WORLD_DIR/starfive-normal-linux
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv visionfive_defconfig
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv dtbs
mv arch/riscv/boot/dts/starfive/jh7100-starfive-visionfive-v1.dtb $SECURE_WORLD_DIR/out/visionfive-v1-uart-reserved-plic-nonet.dtb

git checkout a6fbbbdf
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv visionfive_defconfig
make CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv -j$CPU_NUM
mv arch/riscv/boot/Image.gz $SECURE_WORLD_DIR/out/fedora_with_modules.gz

# build user-level sdk and driver for secure-linux installation
cd $SECURE_WORLD_DIR/starfive-penglai
cd penglai-selinux-driver && make && cp penglai_linux.ko $SECURE_WORLD_DIR/out/ && cd ..
cd penglai-selinux-sdk && make && cp host/host $SECURE_WORLD_DIR/out/

# build secure linux
cd $SECURE_WORLD_DIR/starfive-secure-linux
make -j$CPU_NUM
cp work/linux/arch/riscv/boot/Image $SECURE_WORLD_DIR/out/sec-image
cp work/linux/arch/riscv/boot/dts/starfive/jh7100-starfive-visionfive-v1.dtb $SECURE_WORLD_DIR/out/sec-dtb.dtb