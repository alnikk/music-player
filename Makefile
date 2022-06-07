all:
	gcc -o main main.c $(shell pkg-config --cflags --libs gstreamer-1.0)
