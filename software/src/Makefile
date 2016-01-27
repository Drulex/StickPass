# Makefile

DEVICE     = attiny85
F_CPU      = 16500000
PROGRAMMER = usbtiny
AVRDUDE    = avrdude -c $(PROGRAMMER) -p $(DEVICE)


help:
	@echo "Use one of the following rules:"
	@echo "    make test ...... To make sure programmer can read the target correctly."
	@echo "    make hex ....... To compile program and build hex file."
	@echo "    make flash ..... To flash the firmware on the target."
	@echo "    make clean ..... To remove all object and hex files."



# Simple test routine to make sure programmer can read the attiny85 chip
test:
	$(AVRDUDE)


hex:
	@echo "Not yet implemented."


flash:
	@echo "Not yet implemented."


clean:
	@echo "Not yet implemented."
