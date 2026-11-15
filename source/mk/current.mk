# Current production composition. Values/order copied from the accepted R20 build.
# R7_SHELL_MODE is replaced by the unconditional production main; compatibility retains it.
CURRENT_D ?= canonical
RESOURCE_FLAGS = $(if $(QDOS_CFG_MAX_JOBS),-DQDOS_CFG_MAX_JOBS=$(QDOS_CFG_MAX_JOBS)) $(if $(QDOS_CFG_MAX_CPUS),-DQDOS_CFG_MAX_CPUS=$(QDOS_CFG_MAX_CPUS)) $(if $(QDOS_CFG_MAX_SESSIONS),-DQDOS_CFG_MAX_SESSIONS=$(QDOS_CFG_MAX_SESSIONS))
RESOURCE_FLAGS += $(if $(filter 1,$(R4_EXPERIMENTAL_64)),-DQDOS_R4_EXPERIMENTAL_64)
CURRENT_FLAGS = $(RESOURCE_FLAGS) -DQDOS_R4_RESOURCES -DQDOS_LAYOUT_R9 -DQDOS_FS_WRITE -DQDOS_APP_STACK=65536u -DQDOS_VCON -DQDOS_JOBCTL -DQDOS_VFAT -DQDOS_PAGING -DQDOS_DYNPROC -DQDOS_DYNPROC_SHELL -DQDOS_ENV -DQDOS_TITLES -DQDOS_SYNC -DQDOS_TASKMGR -DQDOS_SURFACES -DQDOS_R13A -DQDOS_R13B -DQDOS_R14 -DQDOS_R16 -DQDOS_R16B -DQDOS_R17 -DQDOS_R18 -DQDOS_R20 $(R20_EXTRA)
CURRENT_EXTRA_OBJS = proc.o frames.o vas.o pheap.o vcon.o jobctl.o env.o pathres.o sync.o taskmgr.o surface.o gsurf.o fontreg.o font_recovery.o font_console.o kev.o vgapres13a.o rtc.o uart.o uart_s17.o bkl.o bkl_s.o smp17.o smp_s17.o mbr.o r16b_s.o memmap.o r20w.o
CURRENT_CTX = kctx17.o
CURRENT_FLAGS += $(if $(filter 1,$(APOLLO_R6)),-DQDOS_APOLLO_R6)
CURRENT_EXTRA_OBJS += $(if $(filter 1,$(APOLLO_R6)),r6boot.o)
CURRENT_FLAGS += $(if $(filter 1,$(APOLLO_R7)),-DQDOS_APOLLO_R7)
