export SHORTNAME	:= VNDS
export LONGNAME 	:= Visual Novel Reader
export VERSION 		:= 1.4.9
ICON 		:= -b logo.bmp
EFS			:= $(CURDIR)/tools/efs/efs

#------------------------------------------------------------------------------
.SUFFIXES:
#------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

export TARGET := $(shell basename $(CURDIR))
export TOPDIR := $(CURDIR)

.PHONY: $(TARGET).arm7 $(TARGET).arm9

#------------------------------------------------------------------------------
# main targets
#------------------------------------------------------------------------------
all: $(TARGET).nds $(TARGET)-EFS.nds

#------------------------------------------------------------------------------

dist: $(TARGET).nds
	rm -rf vnds
	@mkdir vnds
	cp $(TARGET).nds vnds/
	make clean
	cp -r src/ vnds
	cp -r skins/ vnds
	cp -r manual/ vnds
	#cp -r tools/ vnds
	cp config.ini vnds
	cp _default.ttf vnds
	cp license.txt vnds
	cp README vnds
	cp logo.bmp vnds
	#cp xmlParser-license.txt vnds
	cp ChangeLog vnds
	cp Makefile vnds
	mkdir vnds/novels
	find vnds/ -type d -name ".svn" -depth -exec rm -rf '{}' \;
	tar czvf vnds-$(VERSION).tar.gz vnds/

$(TARGET)-EFS.nds: $(TARGET).nds
	@ndstool -c $(TARGET)-EFS.nds -7 $(TARGET).arm7 -9 $(TARGET).arm9 $(ICON) "$(SHORTNAME);$(LONGNAME);$(VERSION)" -d $(CURDIR)/vnds
	@$(EFS) $(notdir $@)

$(TARGET).nds: $(TARGET).arm7 $(TARGET).arm9
	@ndstool -c $(TARGET).nds -7 $(TARGET).arm7 -9 $(TARGET).arm9 $(ICON) "$(SHORTNAME);$(LONGNAME);$(VERSION)"

#------------------------------------------------------------------------------
$(TARGET).arm7	: arm7/$(TARGET).elf
$(TARGET).arm9	: arm9/$(TARGET).elf

#------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C src/arm7
	
#------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C src/arm9

#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
clean:
	$(MAKE) -C src/arm9 clean
	$(MAKE) -C src/arm7 clean
	rm -f $(TARGET).ds.gba $(TARGET).nds $(TARGET)-EFS.nds $(TARGET).arm7 $(TARGET).arm9
