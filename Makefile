make:
	gcc -std=gnu99 -pedantic -lpthread -ltr -o main main.c

clean:
	rm -f proj2.out
	rm -f main


