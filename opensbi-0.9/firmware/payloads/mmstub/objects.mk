#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2019 Western Digital Corporation or its affiliates.
# Copyright (c) 2023, IPADS Lab. All rights reserved.
#

firmware-bins-$(FW_PAYLOAD) += payloads/mmstub/mmstub.bin

mmstub-y += mmstub_head.o
mmstub-y += mmstub_main.o

%/mmstub.o: $(foreach obj,$(mmstub-y),%/$(obj))
	$(call merge_objs,$@,$^)

%/mmstub.dep: $(foreach dep,$(mmstub-y:.o=.dep),%/$(dep))
	$(call merge_deps,$@,$^)
