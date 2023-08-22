#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_SUSPEND) += suspend/fdt_suspend.o
libsbiutils-objs-$(CONFIG_FDT_SUSPEND) += suspend/fdt_suspend_drivers.o
