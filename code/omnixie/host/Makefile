all : omnixie_host

RV003USB:=../../rv003usb
RAWDRAW:=../../rawdraw

omnixie_host : omnixie_host.c
	gcc -o $@ $^ -I$(RV003USB)/ch32fun/minichlink -I$(RAWDRAW) \
		-lX11 -lXinerama -lXext -lGL -lpthread -lm -ludev -g -Wall

clean :
	rm -rf omnixie_host
