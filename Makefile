CC=gcc
CX=g++

.PHONY: all

all: sut serial_rowre

sut: serial_util.c
	$(CC) -o sut serial_util.c

serial_rowre: serial_rowre.cpp
	$(CX) -o sre serial_rowre.cpp
