OBJ = args.o timer.o wrap.o packet.o file.o
STD = -std=c99
main: $(OBJ) main.o
	gcc $(STD) -pthread -g -D_POSIX_C_SOURCE=200809L  $(OBJ) main.o -o main
main.o:$(OBJ) 
	gcc $(STD) -c $(OBJ) main.c
args.o: args.h args.c 
	gcc -c args.c
timer.o: timer.h timer.c
	gcc -c timer.c
wrap.o:wrap.h wrap.c
	gcc -c wrap.c
packet.o:packet.h packet.c file.o
	gcc $(STD) -c packet.c file.o
file.o:file.h file.c
	gcc $(STD) -c file.c 
clean:
	rm $(OBJ)
