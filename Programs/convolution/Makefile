# The PROGRAM variable should be the name of the final executable
PROGRAM = convolution.x
PROGRAM_GKLEE = gklee_convolution.x

# List your .cc-files here
SRCS = convolution.cu


LIBDIR =
LIBS =
LDFLAGS = -g

#######################Normal nvcc compilation
CC = nvcc
OPT = -O0 -std=c++11
#######################
#######################GKLEE compilation
GKCC = gklee-nvcc
GKLEE = gklee
GKOPT = -O0
#######################
CFLAGS = --Wno-deprecated-gpu-targets

INCDIR= -I.

OBJS = $(SRCS:.cu=.o)
GKLEE_INTER = $(SRCS:.cu=.cpp)

.SUFFIXES: .cu .o .x

all: clean normal gklee

gklee:
	$(GKCC) -DGKLEE_MODE=1 $(GKOPT) $(LDFLAGS) $(INCDIR) $(LIBDIR) -o $(PROGRAM_GKLEE) $(SRCS) $(LIBS)

run-gklee:
	#$(GKLEE) -check-BC ./$(PROGRAM_GKLEE)
	time $(GKLEE) ./$(PROGRAM_GKLEE) 2>&1 | tee GKLEE_OUTPUT.log
normal:
	$(CC) $(OPT) $(CFLAGS) $(LDFLAGS) $(INCDIR) $(LIBDIR) -o $(PROGRAM) $(SRCS) $(LIBS)

run-normal:
	./$(PROGRAM) 2>&1 | tee NORMAL_OUTPUT.log

.cu.o:
	$(CC) $(OPT) -c $(CFLAGS) $(INCDIR) $< -o $@

clean:
	rm -f $(OBJS) $(PROGRAM) *~
	rm -f $(GKLEE_INTER) $(PROGRAM_GKLEE) *.dot *.kernelSet.txt *~
	rm -f *.log
	rm -f dump_path_*
