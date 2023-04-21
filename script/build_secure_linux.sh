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
