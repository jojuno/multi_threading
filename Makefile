#
# This is the Makefile that can be used to create the "warmup1" executable
# To create "warmup1" executable, do:
#	make warmup1
#

warmup2: warmup2.o 
	gcc -o warmup2 -g warmup2.o -lpthread

warmup2.o: warmup2.c
	gcc -g -c -Wall warmup2.c 

clean:
	rm -f *.o warmup2









