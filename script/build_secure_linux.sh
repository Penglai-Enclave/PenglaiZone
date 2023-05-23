if test -z "$SECURE_WORLD_DIR";then
	echo "SECURE_WORLD_DIR is null string"
    exit 1
else
	echo $SECURE_WORLD_DIR
fi

# get hart number
CPU_NUM=$(cat /proc/cpuinfo| grep "processor"| wc -l)


# build secure linux
cd $SECURE_WORLD_DIR/starfive-secure-linux
test -d work && make clean
make -j$CPU_NUM
cp work/sec-image $SECURE_WORLD_DIR/out/secure_linux/sec-image
cp work/sec-dtb.dtb $SECURE_WORLD_DIR/out/secure_linux/sec-dtb.dtb

# sign the secure linux
cd $SECURE_WORLD_DIR/starfive-penglai/penglai-selinux-sdk/sign_tool
cp $SECURE_WORLD_DIR/out/secure_linux/sec-image test_dir
cp $SECURE_WORLD_DIR/out/secure_linux/sec-dtb.dtb test_dir
make
cd test_dir
../penglai_sign sign \
        -image sec-image -imageaddr 0xc0200000 \
        -dtb sec-dtb.dtb -dtbaddr 0x186000000 \
        -key SM2PrivateKey.pem -out ccs-file
cp ccs-file $SECURE_WORLD_DIR/out/secure_linux/css-file
