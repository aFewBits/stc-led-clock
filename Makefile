SDCC ?= sdcc
STCCODESIZE ?= 8000
SDCCOPTS ?= --iram-size 256 --code-size $(STCCODESIZE) --data-loc 0x30 --disable-warning 158
SRC = src/adc.c src/ds1302.c src/timer.c src/display.c src/utility.c src/serial.c src/sound.c
OBJ=$(patsubst src%.c,build%.rel, $(SRC))

.PHONY : doall

all: main

build/%.rel: src/%.c src/%.h doall
	mkdir -p $(dir $@)
	$(SDCC) $(SDCCOPTS) -o $@ -c $<

main: $(OBJ)
	$(SDCC) -o build/ src/$@.c $(SDCCOPTS) $^
	cp build/$@.ihx $@.hex
	awk -f lastadr.awk main.hex

clean:
	rm -f *.ihx *.hex *.bin
	rm -rf build/*
