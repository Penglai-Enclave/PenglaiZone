#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_HSM) += hsm/fdt_hsm.o
libsbiutils-objs-$(CONFIG_FDT_HSM) += hsm/fdt_hsm_drivers.o

carray-fdt_hsm_drivers-$(CONFIG_FDT_HSM_RPMI) += fdt_hsm_rpmi
libsbiutils-objs-$(CONFIG_FDT_HSM_RPMI) += hsm/fdt_hsm_rpmi.o
