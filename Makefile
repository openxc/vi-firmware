all:
	cd src && $(MAKE)

upload:
	cd src && $(MAKE) $@

emulator:
	cd src && EMULATOR=1 $(MAKE)

clean:
	cd src && $(MAKE) $@
