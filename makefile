TARGET = ww
CC     = gcc
$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM
