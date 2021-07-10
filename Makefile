loader64: loader64.c gopt/gopt.c gopt/gopt.h
	gcc -Wall loader64.c gopt/gopt.c -o loader64 -lusb-1.0 -lftdi1

clean:
	rm -f loader64
