make:
	gcc -std=gnu99 -pedantic -lpthread -lrt -o main main.c

clean:
	rm -f proj2.out
	rm -f xdetkod00_all_aboard
	rm -f xdetkod00_bus
	rm -f xdetkod00_multiplex
	rm -f xdetkod00_mutex




#gcc -std=gnu99 -Wall -Wextra -Werror -pedantic -o main main.c
