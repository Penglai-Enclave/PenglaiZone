#!/bin/bash
cd /home/penglai/penglai-enclave/Penglai-Enclave-sPMP
rm -r build

# success
make FW_PIC=n PLATFORM=generic CROSS_COMPILE=riscv64-unknown-linux-gnu- FW_PAYLOADMM=y FW_PAYLOADMM_PATH=/home/penglai/penglai-enclave/edk2-staging/STANDALONE_MM.fd

# docker run -v $(pwd):/home/penglai/penglai-enclave -w /home/penglai/penglai-enclave --rm -it ddnirvana/penglai-enclave:v0.5 ./build_opensbi1.3.sh
