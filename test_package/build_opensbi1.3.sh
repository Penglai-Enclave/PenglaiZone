#!/bin/bash
rm -r build
CROSS_COMPILE=riscv64-linux-gnu- make PLATFORM=generic FW_PAYLOAD_PATH=/home/sqy/intel/rpmi/Build/RiscVVirtQemu/DEBUG_GCC5/FV/RISCV_VIRT_CODE.fd FW_PAYLOADMM_PATH=/home/sqy/intel/rpmi/STANDALONE_MM.fd
