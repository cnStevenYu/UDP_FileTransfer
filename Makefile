OBJ = args.o timer.o wrap.o packet.o file.o host.o
STD = -std=c99
DEG = -g
main: $(OBJ) main.o
	gcc $(STD) $(DEG) -pthread -D_POSIX_C_SOURCE=200809L  $(OBJ) main.o -o main
main.o:$(OBJ) 
	gcc $(STD) $(DEG) -c $(OBJ) main.c
args.o: args.h args.c 
	gcc $(STD) $(DEG) -c args.c
timer.o: timer.h timer.c
	gcc $(STD) $(DEG) -c timer.c
wrap.o:wrap.h wrap.c
	gcc $(STD) $(DEG) -c wrap.c
packet.o:packet.h packet.c file.o
	gcc $(STD) $(DEG) -c packet.c file.o
file.o:file.h file.c
	gcc $(STD) $(DEG) -c file.c 
host.o:host.h host.c
	gcc $(STD) $(DEG) -c host.c
clean: 
	rm $(OBJ)
