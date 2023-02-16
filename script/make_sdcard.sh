while getopts "d:" arg
do
    case $arg in
        d)
        DEVICE=$OPTARG
        ;;
        ?)
        echo "unkonw argument"
        exit 1
        ;;
    esac
done

if [ ! $DEVICE ]
then
    DEVICE=/dev/sda
fi
echo "SD card device: $DEVICE"
if [ ! -e $DEVICE ]
then
    echo 'Device not found!'
    exit 1
fi

if test -z "$SECURE_WORLD_DIR";then
	echo "SECURE_WORLD_DIR is null string"
    exit 1
else
	echo $SECURE_WORLD_DIR
fi

sudo apt-get update
sudo apt-get install zstd

mkdir -p $SECURE_WORLD_DIR/download && cd $SECURE_WORLD_DIR/download
wget --no-check-certificate https://fedora.starfivetech.com/pub/downloads/VisionFive-release/Fedora-riscv64-jh7100-developer-xfce-Rawhide-20211226-214100.n.0-sda.raw.zst
zstd -d Fedora-riscv64-jh7100-developer-xfce-Rawhide-20211226-214100.n.0-sda.raw.zst
sudo dd if=Fedora-riscv64-jh7100-developer-xfce-Rawhide-20211226-214100.n.0-sda.raw of=$DEVICE bs=8M status=progress && sync
