CRUFT = *.elf *.hex *.lst *.o *.as

all: clean experiment upload

experiment:
	avr-gcc -std=c99 -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o experiment.o experiment.c
	avr-gcc -std=c99 -mmcu=atmega328p experiment.o -o experiment.elf
	avr-objcopy -O ihex -R .eeprom experiment.elf experiment.hex

assembly:
	avr-gcc -std=c99 -Os -DF_CPU=16000000UL -mmcu=atmega328p -S -o experiment.as experiment.c

upload:
	sudo avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:experiment.hex

clean:
	rm -f $(CRUFT)
