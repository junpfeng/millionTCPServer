# source code
src = $(wildcard *.cpp)

# target name
targets = $(patsubst %.cpp, %, $(src))

# complier
CC = g++

# header file
hdir = ../dependence/include

# flags
CFLAGS = -I$(hdir) -Wall -g -std=c++11 -lpthread 

all:$(targets)

$(targets):%:%.cpp
	$(CC) $< -o $@ $(CFLAGS)

.PHONY:clean all
clean:
	-rm -rf $(target)


