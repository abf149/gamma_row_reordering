CC=gcc
CX=g++
PCC=opencilk-clang
PCX=opencilk-clang++

.PHONY: all

all: sut serial_rowre parallel_rowre random_csr parallel_intersection

sut: serial_util.c
	$(CC) -o sut serial_util.c

serial_rowre: serial_rowre.cpp
	$(CX)  -o sre serial_rowre.cpp

parallel_rowre: parallel_rowre.cpp
	$(PCX) -o pre -fopencilk -O2 -g3 -mavx -march=skylake parallel_rowre.cpp

parallel_intersection: parallel_intersection.cpp
	$(PCX) -o pin -fopencilk -O2 -g3 -mavx -march=skylake parallel_intersection.cpp

random_csr: random_csr.cpp
	$(CX) -std=c++11 -o rcsr random_csr.cpp
