main: main.c wrap.c
	gcc -std=c99 -pthread -g main.c wrap.c -o main
