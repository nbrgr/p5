#the C compiler
CC = gcc

#compiler flags
# -O     optimizes the code
# -Wall  turns on compiler warnings
CFLAGS = -O -Wall -std=gnu99 

#target executable
TARGET = fscheck

all: $(TARGET)

#make an executable from whoosh.c
$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) fscheck
