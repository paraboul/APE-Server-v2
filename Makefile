 SUBDIRS = src/core/
 
.PHONY: $(SUBDIRS)
 
$(SUBDIRS):
	$(MAKE) -w -C $@ $(MAKECMDGOALS)