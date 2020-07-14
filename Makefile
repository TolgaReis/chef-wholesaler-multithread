program: program.c
	gcc -o program program.c -pthread -lrt
clean:
	rm -f program
