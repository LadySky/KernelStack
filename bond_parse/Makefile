TAR=bond_parse_test
OBJ=bond_parse.o 
CC=gcc
RM=rm -rf
CFLAGS= -Wall -O2

$(TAR): $(OBJ) main.o
	$(CC) $(CFLAGS) $(OBJ) main.o -o $@

main.o: %.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ): %.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@
	
.PHONY:clean
clean:
	$(RM) $(OBJ) $(TAR)