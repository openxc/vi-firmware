all:
	cd cantranslator && $(MAKE)

upload:
	cd cantranslator && $(MAKE) $@

emulator:
	cd canemulator && $(MAKE)

clean:
	cd cantranslator && $(MAKE) $@
	cd canemulator && $(MAKE) $@
