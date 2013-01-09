# Makefile is used to build the application
# See README for more details
# Dept. of Computer Science, University at Buffalo
# Project - 1
# Authors: Sanket Kulkarni, Sree Harsha Konduri, Rohit Ghoshal
# 2012

web: web.c
	gcc -o web web.c -lpthread -I.
clean:
	rm -f *.o

