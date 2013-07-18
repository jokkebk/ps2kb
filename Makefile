CC = avr-gcc
OBJCOPY = avr-objcopy
DUDE = avrdude

CFLAGS = -O2 -mmcu=attiny2313
OBJFLAGS = -j .text -j .data -O ihex
DUDEFLAGS = -p attiny2313 -c usbtiny -q

all: ps2.hex

clean:
	$(RM) *.o *.elf *.hex

run: ps2.flash

%.flash: %.hex
	$(DUDE) $(DUDEFLAGS) -U flash:w:$*.hex
	
%.hex: %.elf
	$(OBJCOPY) $(OBJFLAGS) $< $@

ps2.elf: ps2.o ring.o util.o
	$(CC) $(CFLAGS) $^ -o $@
	
%.elf: %.o
	$(CC) $(CFLAGS) $< -o $@
	
%.o: %.c	
	$(CC) $(CFLAGS) -c $< -o $@
	
%.o: %.S	
	$(CC) $(CFLAGS) -c $< -o $@
