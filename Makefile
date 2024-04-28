make:
	gcc -std=gnu99 -pedantic -lpthread -ltr -o proj2 proj2.c

clean:
	rm -f proj2.out
	rm -f proj2

