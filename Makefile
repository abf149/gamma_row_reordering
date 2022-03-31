CC=gcc

sut: serial_util.c
	$(CC) -o sut serial_util.c
