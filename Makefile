# DMR Examples Makefile
# Supports building individual examples or all examples at once

CC = mpicc
DMRFLAGS = -ldmr
CFLAGS = -Wall -g -lm -DDYNRES

# Directories
SRC_DIR = src
COMMON_DIR = $(SRC_DIR)/common
EXAMPLES_DIR = $(SRC_DIR)/examples
INCLUDE_DIR = $(SRC_DIR)/include

# Common source files
COMMON_SOURCES = $(COMMON_DIR)/dmr_utils.c $(COMMON_DIR)/mpi_helpers.c
COMMON_OBJECTS = $(COMMON_SOURCES:.c=.o)

# Include paths
INCLUDES = -I$(COMMON_DIR) -I$(INCLUDE_DIR)

# Available examples (add new examples here)
EXAMPLES = counter timing

# Default target builds all examples
all: $(EXAMPLES)

# Counter example
counter: $(COMMON_OBJECTS) $(EXAMPLES_DIR)/counter/counter.o $(EXAMPLES_DIR)/counter/counter_functions.o
	$(CC) $(CFLAGS) $(DMRFLAGS) $^ -o $@

# Timing example (advanced timing functionality)
timing: $(COMMON_OBJECTS) $(EXAMPLES_DIR)/timing/timing.o $(EXAMPLES_DIR)/timing/timing_functions.o
	$(CC) $(CFLAGS) $(DMRFLAGS) $^ -o $@

# Pattern rule for example object files
$(EXAMPLES_DIR)/%/%.o: $(EXAMPLES_DIR)/%/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -I$(EXAMPLES_DIR)/$* -c $< -o $@

$(EXAMPLES_DIR)/%/%_functions.o: $(EXAMPLES_DIR)/%/%_functions.c
	$(CC) $(CFLAGS) $(INCLUDES) -I$(EXAMPLES_DIR)/$* -c $< -o $@

# Pattern rule for common object files
$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c $(COMMON_DIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(EXAMPLES) 
	rm -f $(COMMON_DIR)/*.o
	rm -f $(EXAMPLES_DIR)/*/*.o
	rm -f slurm-*.out
	rm -f checkpoints/*
	rm -f plans/*
	rm -f results/*
	rm -f nodefile.*

# Clean everything including checkpoints
clean-all: clean
	rm -f checkpoints/*

# Build specific example
build-%:
	$(MAKE) $*

# Show available examples
list:
	@echo "Available examples:"
	@for example in $(EXAMPLES); do echo "  $$example"; done

# Create new example template (usage: make new-example NAME=example_name)
new-example:
	@if [ -z "$(NAME)" ]; then \
		echo "Usage: make new-example NAME=example_name"; \
		exit 1; \
	fi
	@mkdir -p $(EXAMPLES_DIR)/$(NAME)
	@echo "Created directory: $(EXAMPLES_DIR)/$(NAME)"
	@echo "Don't forget to:"
	@echo "  1. Add '$(NAME)' to the EXAMPLES list in this Makefile"
	@echo "  2. Create $(NAME).h, $(NAME).c, and $(NAME)_functions.c files"
	@echo "  3. Add build rule for $(NAME) if needed"

# Help target
help:
	@echo "DMR Examples Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all               Build all examples"
	@echo "  <example>         Build specific example (e.g., 'counter')"
	@echo "  build-<example>   Alternative syntax for building specific example"
	@echo "  list              List available examples"
	@echo "  new-example       Create new example template (use NAME=example_name)"
	@echo "  clean             Remove build artifacts"
	@echo "  clean-all         Remove build artifacts and checkpoints"
	@echo "  help              Show this help message"
	@echo ""
	@echo "Available examples: $(EXAMPLES)"

.PHONY: all clean list new-example help build-%
