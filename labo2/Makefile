CC = arm-linux-gnueabihf-gcc-6.4.1

SOURCE = $(wildcard *.c)
EXECUTABLES = $(patsubst %.c,%,$(SOURCE))
CCFLAGS = -Wall
EXPORTEDIR = /export/drv


all: $(EXECUTABLES) exporte

%: %.c
	$(CC) $< -o $@  $(CCFLAGS)
exporte:
	cp $(EXECUTABLES) /export/drv
clean:
	rm -f $(EXECUTABLES)
	rm -f $(addprefix $(EXPORTEDIR)/,$(EXECUTABLES))
