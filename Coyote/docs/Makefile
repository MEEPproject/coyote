
export ROME_BASE := $(CURDIR)/..

all: doxygen
	@echo Done

doxygen:
	@echo Making Doxygen...
	doxygen > $(CURDIR)/doxygen.log 2>&1

clean:
	rm -Rf html/
	rm -f doxygen_coyote_warnings.out
	rm -f doxygen.log
