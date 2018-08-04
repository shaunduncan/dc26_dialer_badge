all:
	mkdir -p build
	gcc -o build/badge badge.c dialer.c effects.c -lm -lpthread -lwiringPi -lws2811
