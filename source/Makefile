# The root owns the sole current build. Historical profiles are frozen in Git.
.DEFAULT_GOAL := all
ROOT := $(abspath ..)
GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)
.PHONY: $(GOALS)
$(GOALS):
	$(MAKE) -C "$(ROOT)" $@
