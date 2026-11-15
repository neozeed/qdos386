# Root entry points for the authoritative source/ tree.
# source/Makefile owns compilation, flags, linking and image conversion.
.DEFAULT_GOAL := all

HOSTTOOLS ?= $(HOME)/i386vm
I386VM ?= $(HOSTTOOLS)
QDOS386_TOOLS ?= $(I386VM)

.PHONY: all production production64 apollo-r6 current-shell diagnostic r20 r18 help
all: production

production current-shell diagnostic:
	$(MAKE) -C source $@ I386VM="$(I386VM)" QDOS386_TOOLS="$(QDOS386_TOOLS)"

# The normal production profile remains 24 jobs; expanded profiles are explicit.
production64:
	$(MAKE) -C source production current-shell CURRENT_D=jobs64 QDOS_CFG_MAX_JOBS=64 I386VM="$(I386VM)" QDOS386_TOOLS="$(QDOS386_TOOLS)"

apollo-r6:
	$(MAKE) -C source production current-shell CURRENT_D=apollo-r6 QDOS_CFG_MAX_JOBS=64 APOLLO_R6=1 I386VM="$(I386VM)" QDOS386_TOOLS="$(QDOS386_TOOLS)"

.PHONY: apollo-r7
apollo-r7:
	$(MAKE) -C source production current-shell CURRENT_D=apollo-r7 QDOS_CFG_MAX_JOBS=64 APOLLO_R6=1 APOLLO_R7=1 I386VM="$(I386VM)" QDOS386_TOOLS="$(QDOS386_TOOLS)"

.PHONY: apollo-r7-ro
apollo-r7-ro:
	$(MAKE) -C source production current-shell CURRENT_D=apollo-r7-ro QDOS_CFG_MAX_JOBS=64 APOLLO_R6=1 APOLLO_R7=1 APOLLO_FAT32=1 I386VM="$(I386VM)" QDOS386_TOOLS="$(QDOS386_TOOLS)"

r20: production

r18:
	$(MAKE) -C source qdos386-r18-pc I386VM="$(I386VM)" QDOS386_TOOLS="$(QDOS386_TOOLS)"

# Keep other source/ targets accessible without duplicating their recipes here.
# Explicit requested goals avoid pattern rules that try to rebuild this Makefile.
FORWARDED_GOALS := $(filter-out all production production64 apollo-r6 apollo-r7 apollo-r7-ro current-shell diagnostic r20 r18 help,$(MAKECMDGOALS))
ifneq ($(strip $(FORWARDED_GOALS)),)
.PHONY: $(FORWARDED_GOALS)
$(FORWARDED_GOALS):
	$(MAKE) -C source $@ I386VM="$(I386VM)" QDOS386_TOOLS="$(QDOS386_TOOLS)"
endif

help:
	@printf '%s\n' \
	  'make [production] HOSTTOOLS=/path/to/tools  Build current 24-job serial/VGA kernels' \
	  'make apollo-r6 HOSTTOOLS=/path/to/tools     Build R6A 64-job kernels and matching shells' \
	  'make apollo-r7 HOSTTOOLS=/path/to/tools     Build R7 maintenance kernels and matching shells' \
	  'make apollo-r7-ro HOSTTOOLS=/path/to/tools  Build R7 with optional read-only FAT32 WIN0' \
	  'make disk-tools HOSTTOOLS=/path/to/tools    Build native FDISK, FORMAT and CHKDSK' \
	  'make production64 HOSTTOOLS=/path/to/tools Build R5 64-job kernels and matching shells' \
	  'make current-shell HOSTTOOLS=/path/to/tools Build matching native shell/SH' \
	  'make diagnostic HOSTTOOLS=/path/to/tools   Build legacy diagnostic kernel/fixture' \
	  'make r18 HOSTTOOLS=/path/to/tools          Build historical R18 regression kernels' \
	  'Other requested targets are forwarded to source/Makefile.' \
	  'I386VM and QDOS386_TOOLS may be overridden independently.' \
	  'Build different profiles sequentially; their intermediates are shared.'
