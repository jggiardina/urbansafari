
# Copyright (C) 2011 by Jonathan Appavoo, Boston University

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

targets = $(UILIBFILE)

include ../config.mk

#UISYSTEM=$(shell uname)

#ifeq ($(UISYSTEM),Darwin)
#  UIINCDIR = -I/opt/local/include
#  UILIBS = -L/opt/local/lib -lSDLmain -lSDL -lSDL_image -framework Cocoa
#else
#  UINCDIR = 
#  UILIBS = -lSDL
#endif

CFLAGS := -g $(UIINCDIR) -std=c99

$(UILIBFILE): $(objs)
	ar -c -r $(UILIBFILE) $(objs)

#all: uitest
#.PHONY: all

#ttymodse.o: ttymodes.c
#	gcc $(CFLAGS) ttymodes.c -c

#tty.o: tty.c tty.h ttymodes.h
#	gcc $(CFLAGS) tty.c -c

#uistandalone.o: uistandalone.c uistandalone.h
#	gcc ${CFLAGS} uistandalone.c -c

#uitest.o: uitest.c uistandalone.h
#	gcc $(CFLAGS) uitest.c -c 

#uitest: uistandalone.o uitest.o tty.o ttymodes.o
#	gcc $(CFLAGS) uistandalone.o uitest.o tty.o ttymodes.o -o uitest ${UILIBS}

#clean:
#	rm uitest *.o



