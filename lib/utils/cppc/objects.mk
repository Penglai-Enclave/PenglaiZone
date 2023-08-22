#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_CPPC) += cppc/fdt_cppc.o
libsbiutils-objs-$(CONFIG_FDT_CPPC) += cppc/fdt_cppc_drivers.o
