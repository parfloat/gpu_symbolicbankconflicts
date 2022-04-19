# The PROGRAM variable should be the name of the final executable
PROGRAM = AES.x
PROGRAM_GKLEE = gklee_AES.x

# List your .cc-files here
SRCS = aescuda.cu


LIBDIR =
LIBS =
LDFLAGS =

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
	$(GKLEE) ./$(PROGRAM_GKLEE) e 128 small.in.dat key_128_0 2>&1 | tee GKLEE_OUTPUT.log
normal:
	$(CC) $(OPT) $(CFLAGS) $(LDFLAGS) $(INCDIR) $(LIBDIR) -o $(PROGRAM) $(SRCS) $(LIBS)

run-normal-e:
	./$(PROGRAM) e 128 small.in.dat key_128_0 2>&1 | tee NORMAL_OUTPUT_e.log

run-normal-d:
	./$(PROGRAM) d 128 output_0.dat key_128_0 2>&1 | tee NORMAL_OUTPUT_d.log

run-test-normal: run-normal-e run-normal-d



.cu.o:
	$(CC) $(OPT) -c $(CFLAGS) $(INCDIR) $< -o $@

clean:
	rm -f $(OBJS) $(PROGRAM) *~
	rm -f $(GKLEE_INTER) $(PROGRAM_GKLEE) *.dot *.kernelSet.txt *~
	rm -f GKLEE_OUTPUT.log
	rm -f dump_path_*
