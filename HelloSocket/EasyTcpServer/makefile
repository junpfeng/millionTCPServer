src = $(wildcard *.cpp)
targets = $(patsubst %.cpp, %, $(src))

CC = g++
CFLAGS = -Wall -g --std=c++11 -lpthread

all:$(targets)

$(targets):%:%.cpp
	$(CC) $< -o $@ $(CFLAGS)

.PHONY:clean all
clean : 
	rm -rf $(targets)
