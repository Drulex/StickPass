# Makefile

DEVICE     = attiny85
F_CPU      = 16500000
PROGRAMMER = usbtiny
AVRDUDE    = avrdude -c $(PROGRAMMER) -p $(DEVICE)




# Simple test routine to make sure programmer can read the attiny85 chip
test:
	$(AVRDUDE)
