#
# file:    Makefile
#
CFLAGS = -g -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LDLIBS = -lcheck

all: test

# default build rules:
# .c to .o: $(CC) $(CFLAGS) file.c -c -o file.o
# multiple .o to exe. : $(CC) $(LDFLAGS) file.o [file.o..] $(LDLIBS) -o exe

test: test.o qthread.o stack.o switch.o

unit-test: unit-test.o qthread.o stack.o switch.o

clean:
	rm -f *.o test unit-test

test-hw:
	rm -f test
	gcc test.c /home/cs5600/hw2/qthread.c stack.c switch.s -o test `pkg-config --cflags --libs check`
	./test 

qthread:
	rm -f qthread
	gcc qthread.c stack.c switch.s -o qthread `pkg-config --cflags --libs check`
	# ./qthread