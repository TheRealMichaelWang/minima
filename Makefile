define newline


endef

C_SOURCES := $(notdir $(wildcard src/*.c))

all:
	$(foreach C_SOURCE, $(C_SOURCES), gcc src/$(C_SOURCE) -o bin/$(C_SOURCE).o -c $(newline))