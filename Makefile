CC=/opt/m68k-amigaos/bin/m68k-amigaos-gcc
CFLAGS=-O2 -noixemul
INCLUDE=/opt/m68k-amigaos/m68k-amigaos/sys-include/
VASM=vasmm68k_mot
VASMFLAGS=-quiet -Fhunk -phxass -I $(INCLUDE)
DEPS=

%.o: %.s $(DEPS)
	$(VASM) $(VASMFLAGS) -o $@ $<

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mm: mousedriver.o vbrserver.o
	$(CC) $(CFLAGS) -o mm mousedriver.o vbrserver.o
	
clean:
	rm *.o

#rm vbrserver.o
#sudo rm /mnt/arcs/Amiga/vbr
#vasmm68k_mot -Fhunk -phxass -I /opt/m68k-amigaos/m68k-amigaos/sys-include/  -o vbrserver.o vbrserver.s
##vasmm68k_mot -Fhunkexe -spaces -I /opt/m68k-amigaos/m68k-amigaos/sys-include/  -o vbrserver.o vbrserver.s
#/opt/m68k-amigaos/bin/m68k-amigaos-gcc -O2 -noixemul vbr.c vbrserver.o -o vbr
#chmod +x vbr
#sudo cp vbr /mnt/arcs/Amiga
#	
##	Makefile  mousedriver.c  vbrserver.s
