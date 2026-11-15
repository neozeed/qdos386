# Run only in an isolated /tmp working copy. SDK libraries must already exist.
QDOS386_TOOLS ?=
SDK := ../../sdk
KERNEL := ../../kernel
.PHONY: all host
all: FDISK.aout FORMAT.aout CHKDSK.aout
FDISK.aout: main.c $(KERNEL)/fatmeta.c $(KERNEL)/fatmeta.h $(KERNEL)/disktool.h
	QDOS386_TOOLS="$(QDOS386_TOOLS)" $(SDK)/bin/qdos386-gcc -O -I$(KERNEL) -DTOOL_MODE=0 -o $@ $(filter %.c,$^)
FORMAT.aout: main.c $(KERNEL)/fatmeta.c $(KERNEL)/fatmeta.h $(KERNEL)/disktool.h
	QDOS386_TOOLS="$(QDOS386_TOOLS)" $(SDK)/bin/qdos386-gcc -O -I$(KERNEL) -DTOOL_MODE=1 -o $@ $(filter %.c,$^)
CHKDSK.aout: main.c $(KERNEL)/fatmeta.c $(KERNEL)/fatmeta.h $(KERNEL)/disktool.h
	QDOS386_TOOLS="$(QDOS386_TOOLS)" $(SDK)/bin/qdos386-gcc -O -I$(KERNEL) -DTOOL_MODE=2 -o $@ $(filter %.c,$^)
host:
	$(CC) -std=c89 -D_XOPEN_SOURCE=700 -DDISKTOOLS_HOST -Wall -Wextra -I$(KERNEL) -DTOOL_MODE=0 main.c $(KERNEL)/fatmeta.c -o fdisk-host
	$(CC) -std=c89 -D_XOPEN_SOURCE=700 -DDISKTOOLS_HOST -Wall -Wextra -I$(KERNEL) -DTOOL_MODE=1 main.c $(KERNEL)/fatmeta.c -o format-host
	$(CC) -std=c89 -D_XOPEN_SOURCE=700 -DDISKTOOLS_HOST -Wall -Wextra -I$(KERNEL) -DTOOL_MODE=2 main.c $(KERNEL)/fatmeta.c -o chkdsk-host
