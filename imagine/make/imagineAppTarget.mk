CXX_SRC := $(filter %.cxx %.cc %.cpp,$(SRC))
C_SRC := $(filter %.c,$(SRC))
OBJC_SRC := $(filter %.m,$(SRC))
OBJCXX_SRC := $(filter %.mm,$(SRC))
ASM_SRC := $(filter %.s,$(SRC))

CXX_OBJ := $(addprefix $(objDir)/,$(patsubst %.cxx, %.o, $(patsubst %.cpp, %.o, $(CXX_SRC:.cc=.o))))
C_OBJ := $(addprefix $(objDir)/,$(C_SRC:.c=.o))
OBJC_OBJ := $(addprefix $(objDir)/,$(OBJC_SRC:.m=.o))
OBJCXX_OBJ := $(addprefix $(objDir)/,$(OBJCXX_SRC:.mm=.o))
ASM_OBJ := $(addprefix $(objDir)/,$(ASM_SRC:.s=.o))
OBJ += $(CXX_OBJ) $(C_OBJ) $(OBJC_OBJ) $(OBJCXX_OBJ) $(ASM_OBJ)
DEP := $(OBJ:.o=.d)

-include $(DEP)

LDFLAGS += $(LDLIBS)

ifeq ($(ENV), android)
 target := lib$(android_soName).so
endif

targetFile := $(target)$(targetSuffix)$(targetExtension)

# TODO: remove, concatenated compile support no longer used since in GCC 4.6 and Clang 3.0 LTO works
#ifdef O_CONCAT

# concat source target
#ALL_SRC_DEP_BASE := $(allSrc:.cc=.d)
#ALL_SRC_DEP_BASE := $(ALL_SRC_DEP_BASE:.mm=.d)
#ALL_SRC_DEP := $(objDir)/$(ALL_SRC_DEP_BASE)
#-include $(ALL_SRC_DEP)

#$(targetDir)/$(targetFile) : $(genConfigH) $(genMetaH) $(allSrc) $(CXX_SRC) $(OTHER_ALL_OBJ) $(ASM_OBJ)
#	@echo "Compiling/Linking Concatenated $@"
#	@mkdir -p `dirname $@`
#	@mkdir -p `dirname $(ALL_SRC_DEP)`
#	$(PRINT_CMD)$(CC) -o $@ $(allSrc) $(OTHER_ALL_OBJ) $(ASM_OBJ) $(CPPFLAGS) $(CXXFLAGS) $(WHOLE_PROGRAM_CFLAGS) $(LDFLAGS) -MMD -MP -MF $(ALL_SRC_DEP)
#ifeq ($(ENV), iOS)
#ifndef iOSNoCodesign
#	@echo "Signing $@"
#	$(PRINT_CMD)ldid -S $@
#endif
#endif

#else

# standard target
$(targetDir)/$(targetFile) : $(OBJ)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD)$(LD) -o $@ $^ $(LDFLAGS)
ifeq ($(ENV), iOS)
ifndef iOSNoCodesign
	@echo "Signing $@"
	$(PRINT_CMD)ldid -S $@
endif
endif

#endif

ifeq ($(ENV), ps3)

#$(targetDir)/$(target).self: $(targetDir)/$(targetFile)
#	fself.py $< $@

$(targetDir)/pkg/USRDIR/EBOOT.BIN: $(targetDir)/$(targetFile)
	make_self_npdrm $< $@ EM0000-PCEE00000_00-EXPLUSALPHATURBO

#$(targetDir)/pkg/USRDIR/EBOOT.BIN: $(targetDir)/$(targetFile)
#	wine /usr/local/cell/host-win32/bin/make_fself_npdrm $< $@

main: $(targetDir)/pkg/USRDIR/EBOOT.BIN

.PHONY: ps3-pkg

SFOXML ?= $(targetDir)/sfo.xml

$(targetDir)/pkg/PARAM.SFO : $(SFOXML)
	sfo.py -f $< $@

ps3-pkg : $(targetDir)/pkg/PARAM.SFO $(targetDir)/pkg/USRDIR/EBOOT.BIN
	pkg.py --contentid EM0000-PCEE00000_00-EXPLUSALPHATURBO $(targetDir)/pkg/ $(targetDir)/$(target).pkg
	package_finalize $(targetDir)/$(target).pkg

else

main: $(targetDir)/$(targetFile)

endif

.PHONY: cppcheck

cppcheck: $(CXX_SRC) $(C_SRC)
	cppcheck $^ $(CPPFLAGS) -D__GNUC__ -DCHAR_BIT=8
#--check-config
