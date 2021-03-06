CC := gcc-10.2.0
AS := as
LD := gcc-10.2.0

MODE ?= debug

BIN ?= main
ARGS ?=

INCLUDE := inc
SRC     := src
BUILD   := build/$(MODE)

WARNINGS := -Wall -Wextra -Wformat=2 -Wformat-overflow=2 -Winit-self     \
	-Wignored-qualifiers               -Wstrict-aliasing=3           \
	-Walloca -Warray-bounds -Wshadow -Wpointer-arith -Wuninitialized \
	-Wstrict-prototypes -Wredundant-decls -Wunreachable-code         \
	                             -Wunused
LIB      :=
LIBD     :=

ARCH     := lzcnt

CFLAGS = -std=c11 -ggdb3 -fPIC $(addprefix -I,$(INCLUDE)) $(WARNINGS) -fanalyzer $(addprefix -m,$(ARCH))
LDFLAGS = $(addprefix -l,$(LIB)) $(addprefix -L,$(LIBD))

ifeq ($(MODE),release)
	CFLAGS += -O3 -DNDEBUG -flto
else
	CFLAGS += -fsanitize=address,undefined
endif

SOURCES = $(shell find $(SRC) -type f -name "*.c")
HEADERS = $(shell find $(INCLUDE) -type f -name "*.h")

OBJ  = $(patsubst $(SRC)/%,$(BUILD)/%.o,$(SOURCES))
DEP  = $(patsubst $(SRC)/%,$(BUILD)/%.d,$(SOURCES))
PCH  = $(patsubst $(INCLUDE)/%,$(BUILD)/%.gch,$(HEADERS))
OUTD = $(BUILD) $(patsubst $(SRC)/%,$(BUILD)/%,$(shell find $(SRC) -type d))

$(BIN): $(OUTD) $(PCH) $(OBJ)
	$(LD) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

-include $(DEP)

$(BUILD)/%.gch: $(INCLUDE)/%
	$(CC) $(CFLAGS) -MMD -MF $(@:.gch=.d) -c -o $@ $<

$(BUILD)/%.o: $(SRC)/%
	$(CC) $(CFLAGS) -MMD -MF $(@:.o=.d) -c -o $@ $<

$(OUTD):
	-mkdir -p $@

.PHONY: clean run
clean:
	-rm -rf $(BIN) $(BUILD)

run: $(BIN)
	./$(BIN) $(ARGS)
