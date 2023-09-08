nc -z  127.0.0.1 54320 || /usr/bin/gnome-terminal -x ./soc_term.py 54320 &
nc -z  127.0.0.1 54321 || /usr/bin/gnome-terminal -x ./soc_term.py 54321 &
while ! nc -z 127.0.0.1 54320 || ! nc -z 127.0.0.1 54321; do sleep 1; done
./qemu-system-riscv64 -nographic \
  -M virt,pflash0=pflash0,pflash1=pflash1,acpi=off \
  -m 4096 -smp 2 \
  -dtb ./qemu-virt-new.dtb \
  -bios ./Penglai-Enclave-sPMP/build/platform/generic/firmware/fw_dynamic.elf \
  -blockdev node-name=pflash0,driver=file,read-only=on,filename=edk2-staging/RISCV_VIRT_CODE.fd \
  -blockdev node-name=pflash1,driver=file,filename=edk2-staging/RISCV_VIRT_VARS.fd \
  -serial tcp:localhost:54320 -serial tcp:localhost:54321 \
  -device qemu-xhci -device usb-mouse -device usb-kbd \
  -drive file=fat:rw:~/intel/src/fat,id=hd0 -device virtio-blk-device,drive=hd0

# generate device tree
# ./qemu-system-riscv64 -nographic -M virt,dumpdtb=qemu-virt.dtb -m 4096 -smp 2 -bios ./Penglai-Enclave-sPMP/opensbi-0.9/build/platform/generic/firmware/fw_dynamic.elf
