# Variables
CFLAGS  := -Wall 
LFLAGS  := 
CC      := g++ 
BIN     := $(PWD)

# Top Sources
TOP_DIR := $(PWD)
TOP_SRC := $(wildcard $(TOP_DIR)/*.cpp)
TOP_BIN := $(patsubst $(TOP_DIR)/%.cpp,%,$(TOP_SRC))

# Default
all: $(TOP_BIN)

# Clean
clean:
	rm -f $(TOP_BIN)

# Comile applications
%: $(TOP_DIR)/%.cpp 
	$(CC) $(CFLAGS) $(DEF) -o $@ $< $(LFLAGS) 

