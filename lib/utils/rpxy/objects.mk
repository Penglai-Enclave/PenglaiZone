#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_RPXY) += rpxy/fdt_rpxy.o
libsbiutils-objs-$(CONFIG_FDT_RPXY) += rpxy/fdt_rpxy_drivers.o

carray-fdt_rpxy_drivers-$(CONFIG_FDT_RPXY_MBOX) += fdt_rpxy_mbox
libsbiutils-objs-$(CONFIG_FDT_RPXY_MBOX) += rpxy/fdt_rpxy_mbox.o

carray-fdt_rpxy_drivers-$(CONFIG_FDT_RPXY_SPM) += fdt_rpxy_spm
libsbiutils-objs-$(CONFIG_FDT_RPXY_SPM) += rpxy/fdt_rpxy_spm.o
libsbiutils-objs-$(CONFIG_FDT_RPXY_SPM) += rpxy/fdt_rpxy_spm_helper.o
