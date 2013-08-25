CC = avr-gcc
OBJCOPY = avr-objcopy
DUDE = avrdude

MCU = attiny45
CFLAGS = -O2 -mmcu=$(MCU) -DF_CPU=8000000 -DLED_PIN=PB4
#CFLAGS = -O2 -mmcu=$(MCU) -DF_CPU=8000000 -DUSE_BUTTON
OBJFLAGS = -j .text -j .data -O ihex
DUDEFLAGS = -p $(MCU) -c usbtiny -q
OBJECTS = ps2.o ring.o timer.o adc.o main.o
SOURCES = ps2.c ring.c timer.c adc.c main.c

all: ps2.hex

clean:
	$(RM) *.o *.d *.elf *.hex

run: ps2.flash

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

%.flash: %.hex
	$(DUDE) $(DUDEFLAGS) -U flash:w:$*.hex
	
%.hex: %.elf
	$(OBJCOPY) $(OBJFLAGS) $< $@

ps2.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

%.elf: %.o
	$(CC) $(CFLAGS) $< -o $@
	
%.o: %.c	
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) -MM $(CFLAGS) $*.c > $*.d
	
%.o: %.S	
	$(CC) $(CFLAGS) -c $< -o $@
