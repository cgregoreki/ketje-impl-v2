CC			= gcc
PROG_JR 	= test_tiny_ketjeJr
PROG_SR 	= test_tiny_ketjeSr
OBJ_JR 		= test_tiny_ketjeJr.o keccakP200.o tiny_ketjeJr.o
OBJ_SR		= test_tiny_ketjeSr.o keccakP400.o tiny_ketjeSr.o
BIN_JR		= bin/ketje/Jr
BIN_SR		= bin/ketje/Sr
KECCAK_PATH	= src/keccak
JR_PATH		= src/ketje/Jr
SR_PATH		= src/ketje/Sr

OBJDIR 		= bin/obj
#VPATH 		= $(OBJDIR)

OUT_DIR 	= bin bin/obj bin/ketje/Jr bin/ketje/Sr
MKDIR_P 	= mkdir -p

$(shell   mkdir -p $(OUT_DIR))

$(OBJDIR)/%.o : $(KECCAK_PATH)/%.c
	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(JR_PATH)/%.c
	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(SR_PATH)/%.c
	$(CC) -c $< -o $@

OBJPROG_JR = $(addprefix $(BIN_JR)/, $(PROG_JR))
OBJPROG_SR = $(addprefix $(BIN_SR)/, $(PROG_SR))

default: all

all: $(OBJPROG_JR) $(OBJPROG_SR)

$(OBJPROG_JR): $(addprefix $(OBJDIR)/, $(OBJ_JR))
	$(LINK.o) $^ $(LDLIBS) -o $@

$(OBJPROG_SR): $(addprefix $(OBJDIR)/, $(OBJ_SR))
	$(LINK.o) $^ $(LDLIBS) -o $@

clean: 
	@rm -rf bin