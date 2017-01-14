DEVICE = attiny85
F_CPU = 16500000

CFLAGS = -Iusbdrv -Ilibs-device -I.
AVRCC = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o \
	libs-device/osccalASM.o main.o

.c.o:
	$(AVRCC) -c $< -o $@

.S.o:
	$(AVRCC) -x assembler-with-cpp -c $< -o $@

all: main.hex

main.elf: $(OBJECTS) usbconfig.h
	$(AVRCC) -o main.elf $(OBJECTS)

main.hex: main.elf
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.hex

clean:
	rm -f $(OBJECTS) main.elf main.hex
