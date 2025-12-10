CC:=gcc
#CC:=arm-linux-gnueabihf-gcc

OBJECT = input.o output.o hi.o
TARGET = hi
$(TARGET):$(OBJECT)
#hi:input.o output.o hi.o
	$(CC) -o $@ $(OBJECT)
#	gcc -o hi input.o output.o hi.o
#	cp $@ /srv/nfs_server

%.o:%.c
#input.o:input.c
#output.o:output.c
#hi.o:hi.c
	$(CC) -c -o $@ $<
#	gcc -c -o input.o input.c
#	gcc -c -o output.o output.c
#	gcc -c -o hi.o hi.c
	
clean:
	rm -f *.o $(TARGET)
