# OpenCL compile and link flags.
AOCL_COMPILE_CONFIG := $(shell aocl compile-config )
AOCL_LINK_CONFIG := $(shell aocl link-config )

# $(shell aocl link-config )

# Compiler
CXX := g++
CXXFLAGS:=-fPIC -Wno-deprecated-declarations

# Target
TARGET := host

# Directories
INC_DIRS := $(ALTERAOCLSDKROOT)/examples_aoc/common/inc $(AOCL_BOARD_PACKAGE_ROOT)/software/include
LIB_DIRS := 

# Files
SRCS := $(wildcard src/host/*.cpp $(ALTERAOCLSDKROOT)/examples_aoc/common/src/AOCLUtils/*.cpp)
LIBS := rt pthread

# Make it all!
all : $(TARGET)

# Host executable target.
$(TARGET) : Makefile $(SRCS)
	$(CXX) $(CXXFLAGS) $(foreach D,$(INC_DIRS),-I$D) $(AOCL_COMPILE_CONFIG) $(SRCS) $(AOCL_LINK_CONFIG) $(foreach L,$(LIBS),-l$L) -o $(TARGET)

	
# Standard make targets
clean :
	rm -f $(TARGET) *.mon *.aoc*

.PHONY : all clean

