all : rawdraw_test

rawdraw_test : rawdraw_test.c
	gcc -o $@ $^ -lX11 -lpthread -lXinerama -lXext -lGL -g -lm -ldl

clean : rm -rf rawdraw_test