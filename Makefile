# Initial makefile for use of autotools.
CC=g++
FLAGS=`pkg-config --cflags gtk+-2.0 gdk-2.0 libgnome-2.0 libgnomeui-2.0 gdk-pixbuf-xlib-2.0`
HEAD= 
LIBS=`pkg-config --libs gtk+-2.0 gdk-2.0 libgnome-2.0 libgnomeui-2.0 gdk-pixbuf-xlib-2.0`

bin_PROGRAMS=rbfilter
rbfilter_SOURCES=./rbfilter.cpp ./Calcs.cpp ./Rauch.cpp ./Sallen_and_Key.cpp ./Discrete.cpp Calcs.h Drawing.h filter.h preferred.h Discrete.h Enums.h gnome.h rbfilter.h


rbfilter: rbfilter.o ./Calcs.o ./Rauch.o ./Sallen_and_Key.o ./Discrete.o $(HEAD)
	$(CC) -g $(FLAGS) -o rbfilter rbfilter.o ./Calcs.o ./Rauch.o ./Sallen_and_Key.o ./Discrete.o $(LIBS)

%.o: %.cpp $(HEAD)
	$(CC) -g -c $(FLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o
	rm -f rbfilter


