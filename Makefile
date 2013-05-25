CC=gcc
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
PRF=-pg
COP=-W -Wall -Wno-sign-compare -Wno-multichar -Wno-pointer-sign -Wno-parentheses -Wno-missing-field-initializers -Wno-missing-braces -O3
DST=mp4trace etmp4 psnr hist mos miv eg vsgen
INSTDIR=/usr/local/bin

all: $(DST)

mp4trace: bits.o error.o lock.o misc.o queue.o socket.o thread.o timing.o mp4trace.o
	@echo L $@ ...
	@$(CC) $^ -o $@ -lpthread -lgpac_static 

etmp4: bits.o misc.o read.o stat.o writemp4.o etmp4.o
	@echo L $@ ...
	@$(CC) $^ -o $@ -lgpac_static -lm

psnr: psnr.o
	@echo L $@ ...
	@$(CC) $^ -o $@ -lm

hist: stat.o hist.o
	@echo L $@ ...
	@$(CC) $^ -o $@

mos: dir.o mos.o
	@echo L $@ ...
	@$(CC) $^ -o $@

miv: dir.o miv.o
	@echo L $@ ...
	@$(CC) $^ -o $@

eg: misc.o random.o read.o eg.o
	@echo L $@ ...
	@$(CC) $^ -o $@

vsgen: vsgen.o
	@echo L $@ ...
	@$(CC) $^ -o $@

%.o: %.c
	@echo C $< ...
	@$(CC) $(COP) -c $<

install: $(DST)
	@echo I $(DST) in $(INSTDIR) ...
	@install -s -m 755 $(DST) $(INSTDIR)

clean:
	@rm -f $(OBJ) $(DST) gmon.out *.s *.i *~ *.*~ evalvid-2.7.tar.bz2

tar:
	@tar cjf evalvid-2.7.tar.bz2 *.h *.c *.vcproj *.sln Makefile

