shell: shell.o
	gcc -o shell shell.c

shell.o: shell.c
	gcc -c shell.c

clean:
	rm *.o shell
