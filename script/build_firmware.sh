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
