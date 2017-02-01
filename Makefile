# get: gcc -g get.c base64decode.c base64encode.c -o get -lcrypto -loauth -lcurl -Og -foptimize-sibling-calls -fno-stack-protector -z execstack

CC = gcc
CFLAGS = -Wall -lcrypto -loauth -lcurl -foptimize-sibling-calls -fno-stack-protector -z execstack
DEPS = umamicrypt.h aesdecrypt.h aesencrypt.h base64encode.h base64decode.h
OBJ = get.o umamicrypt.o base64encode.o base64decode.o aesencrypt.o aesdecrypt.o

# Assemble but don't link the object files
# when a c file or its dependencies change
%.o: %.c $(DEPS)
	$(CC) -g3 -c -o $@ $< $(CFLAGS)

# Link the object files
get: $(OBJ)
	gcc -g3 -o $@ $^ $(CFLAGS)
