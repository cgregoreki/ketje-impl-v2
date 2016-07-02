CC			= gcc

#
PROG_JR 	= test_tiny_ketjeJr
PROG_SR 	= test_tiny_ketjeSr
OBJ_JR 		= test_tiny_ketjeJr.o keccakP200.o tiny_ketjeJr.o
OBJ_SR		= test_tiny_ketjeSr.o keccakP400.o tiny_ketjeSr.o
BIN_JR		= bin/ketje/Jr
BIN_SR		= bin/ketje/Sr
KECCAK_PATH	= src/keccak
JR_PATH		= src/ketje/Jr
SR_PATH		= src/ketje/Sr

# reference
REF_JR_PATH	= src/reference_cut/ketje/Jr
REF_SR_PATH	= src/reference_cut/ketje/Sr
BIN_REF_JR	= bin/reference_cut/ketje/Jr
BIN_REF_SR	= bin/reference_cut/ketje/Sr
OBJ_REF_JR	= ketjeJr.o
OBJ_REF_SR	= ketjeSr.o
PROG_REF_JR	= ketjeJr
PROG_REF_SR	= ketjeSr

# objects folder
OBJDIR 		= bin/obj

# where the outputs will be
OUT_DIR 	= bin bin/obj bin/ketje/Jr bin/ketje/Sr bin/reference_cut/ketje/Jr bin/reference_cut/ketje/Sr

MKDIR_P 	= mkdir -p

$(shell   mkdir -p $(OUT_DIR))

$(OBJDIR)/%.o : $(KECCAK_PATH)/%.c
	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(JR_PATH)/%.c
	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(SR_PATH)/%.c
	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(REF_JR_PATH)/%.c
	$(CC) -c $< -o $@

$(OBJDIR)/%.o : $(REF_SR_PATH)/%.c
	$(CC) -c $< -o $@

OBJPROG_REF_JR = $(addprefix $(BIN_REF_JR)/, $(PROG_REF_JR))
OBJPROG_REF_SR = $(addprefix $(BIN_REF_SR)/, $(PROG_REF_SR))

OBJPROG_JR = $(addprefix $(BIN_JR)/, $(PROG_JR))
OBJPROG_SR = $(addprefix $(BIN_SR)/, $(PROG_SR))

default: all

all: $(OBJPROG_JR) $(OBJPROG_SR) $(OBJPROG_REF_JR) $(OBJPROG_REF_SR)

$(OBJPROG_JR): $(addprefix $(OBJDIR)/, $(OBJ_JR))
	$(LINK.o) $^ $(LDLIBS) -o $@

$(OBJPROG_SR): $(addprefix $(OBJDIR)/, $(OBJ_SR))
	$(LINK.o) $^ $(LDLIBS) -o $@

$(OBJPROG_REF_JR): $(addprefix $(OBJDIR)/, $(OBJ_REF_JR))
	$(LINK.o) $^ $(LDLIBS) -o $@

$(OBJPROG_REF_SR): $(addprefix $(OBJDIR)/, $(OBJ_REF_SR))
	$(LINK.o) $^ $(LDLIBS) -o $@

clean: 
	@rm -rf bin