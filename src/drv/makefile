-include ../../config.mk

SRC := dht11/dht11.cpp \
	dht22/dht22.cpp \
	di/di.cpp \
	ds18b20/ds18b20.cpp \
	enc/enc.cpp \
	hd44780/hd44780.cpp \
	onewire/onewire.cpp \
	sd/sd_spi.cpp \
	sd/sd.cpp \
	singlewire/singlewire.cpp

INC :=
DEF :=
C_CPP_FLAGS :=
CFLAGS :=
CPPFLAGS :=
AFLAGS :=

LIBDIR :=
LIB :=
LINKED_OBJ :=

# -------------------------

OBJ := $(patsubst %.c,$(OBJDIR)/%.o,$(filter %.c,$(SRC))) \
	$(patsubst %.cpp,$(OBJDIR)/%.o,$(filter %.cpp,$(SRC))) \
	$(patsubst %.s,$(OBJDIR)/%.o,$(filter %.s,$(SRC))) \
	$(patsubst %.S,$(OBJDIR)/%.o,$(filter %.S,$(SRC)))

INC := $(addprefix -I,$(GLOBAL_INC) $(INC))
DEF := $(addprefix -D,$(GLOBAL_DEF) $(DEF))
C_CPP_FLAGS := $(strip $(GLOBAL_C_CPP_FLAGS) $(C_CPP_FLAGS))
CFLAGS := $(strip $(GLOBAL_CFLAGS) $(CFLAGS))
CPPFLAGS := $(strip $(GLOBAL_CPPFLAGS) $(CPPFLAGS))
AFLAGS := $(strip $(GLOBAL_AFLAGS) $(AFLAGS))

# Prevent including this part to main makefile
ifneq ($(ROOT),.)

all: $(OBJ)

clean:
	$(call RMDIR,$(OBJDIR))

.SECONDEXPANSION:
$(OBJDIR)/%.o: %.c | $$(@D)/.f
	$(CC) $(DEF) $(INC) $(C_CPP_FLAGS) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/%.o: %.cpp | $$(@D)/.f
	$(CPP) $(DEF) $(INC) $(C_CPP_FLAGS) $(CPPFLAGS) -c $^ -o $@

$(OBJDIR)/%.o: %.s | $$(@D)/.f
	$(AS) $(DEF) $(INC) $(AFLAGS) -c $^ -o $@

$(OBJDIR)/%.o: %.S | $$(@D)/.f
	$(AS) $(DEF) $(INC) $(AFLAGS) -c $^ -o $@

%/.f:
	$(call MKDIR,$(dir $@))

endif
