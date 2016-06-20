CC			= gcc
PROG_JR 	= test_tiny_ketjeJr
OBJ_JR 		= test_tiny_ketjeJr.o keccakP200.o tiny_ketjeJr.o
BIN_JR		= bin/ketje/JR
KECCAK_PATH	= src/keccak
JR_PATH		= src/ketje/Jr

OBJDIR 		= bin/obj
VPATH 		= $(OBJDIR)

OUT_DIR 	= bin bin/obj bin/ketje/JR
MKDIR_P = mkdir -p

#.PHONY: directories

$(shell   mkdir -p $(OUT_DIR))


#$(OBJDIR)/keccakP200.o : $(KECCAK_PATH)/keccakP200.c
#	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(KECCAK_PATH)/%.c
	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(JR_PATH)/%.c
	$(CC) -c $< -o $@

OBJPROG_JR = $(addprefix $(BIN_JR)/, $(PROG_JR))

default: all

all: $(OBJPROG_JR)

$(OBJPROG_JR): $(addprefix $(OBJDIR)/, $(OBJ_JR))
	$(LINK.o) $^ $(LDLIBS) -o $@
