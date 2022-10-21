# Makefile for building data structures in mods/;
#
INCLUDE_DIR  := include

TESTS_DIR    :=  tests/
OUT_DIR      :=  out/
OUT_BIN      :=  $(OUT_DIR)/test

# Flag to only run tests when objects are (re)built
RUN_FLAG     := $(OUT_DIR)/.runtest

# Copy all public headers out of ./include/ and into $(STAGING_HEADERS);
# include/ may contain subdirectories, but they should reflect what is 
# meant to be in and be copied into STAGING_HEADERS; that is,
# include/tarp/* will end up in <STAGING_HEADERS>/include/tarp/;
# <STAGING_HEADER>/include represents the root search path for
# public headers.
define INSTALL_HEADERS
	cp -urf $(INCLUDE_DIR)/* $(STAGING_HEADERS)/
endef

#
# Find .c source files in any subdirectory under the CWD
# at any nesting level
define FIND_SOURCES
 $(eval SOURCES := $(shell find . -type f -iname "*.c"))
 $(eval SOURCES += $(shell find $(TARP_COHORT_PATH) -type f -iname "*.c"))
endef

#
# Convert list of .c files to list of .o files
define FIND_OBJS
  $(eval OBJS   :=   $(SOURCES:.c=.o))
endef

#
# Convert list of .c file to list of .d files
define FIND_DEPS
  $(eval DEPS   :=   $(SOURCES:.c=.d))
endef


.PHONY : all prepare depends build 

$(FIND_SOURCES)
$(FIND_OBJS)
$(FIND_DEPS)

#$(info SOURCES is $(SOURCES))
#$(info OBJS is $(OBJS))
#$(info DEPS is $(DEPS))
#$(info INCLUDE_FLAGS is $(INCLUDE_FLAGS))
#$(info CPPFLAGS is $(CPPFLAGS))

all: prepare depends build runtest package

prepare :
	@rm -f $(RUN_FLAG)
	$(INSTALL_HEADERS)

build : $(OUT_BIN)

# generate dependency list
depends: $(DEPS)

%.d : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM $^ -MT $(@:.d=.o) -MF $@
-include $(DEPS)

# build objects from sources
$(OUT_BIN) : $(OBJS)
	@touch $(RUN_FLAG)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDDFLAGS) $^ -o $@

runtest: $(OBJS)
	@ if [ -e "$(RUN_FLAG)" ]; then \
		echo "\n[ ] Running Validation tests ..." ; \
		$(OUT_BIN); \
	  fi

%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDDFLAGS) -c $< -o $@

package:
	@echo "\n [ ] Preparing files for packaging ..."
