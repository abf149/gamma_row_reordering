CC=gcc
CX=g++

.PHONY: all

all: sut serial_rowre random_csr

sut: serial_util.c
	$(CC) -o sut serial_util.c

serial_rowre: serial_rowre.cpp
	$(CX) -o sre serial_rowre.cpp

random_csr: random_csr.cpp
	$(CX) -std=c++11 -o rcsr random_csr.cpp
