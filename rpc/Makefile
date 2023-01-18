# Compiler, gcc for C program
CC = gcc

#   We provide the following Flags
#     location independent
#     optimization level 3
#     debugger
#     turn all optional warnings
#     turn all warnings into errors
#     set standard c to c11
CFLAGS = -fPIC -O3 -g -Wall -Werror -std=c11

# some hardcoded path for the project
INCLUDE_PATH = -I../include
LIBRARY_PATH = -L../lib

# the target files and library for the project
SOCKET_FILE = socket.c
SOCKET_O = socket.o

MYLIB_FILE = mylib.c
MYLIB_O = mylib.o
MYLIB = mylib.so

SERVER_FILE = server.c
SERVER = server

# threading needed for server to serve concurrent requests
LINK_PTHREAD   = -lpthread

all: $(MYLIB) $(SERVER)

#gcc -Wall -fPIC -DPIC -c mylib.c && ld -shared -o mylib.so mylib.o -ldl

$(SOCKET_O) : $(SOCKET_FILE)
	$(CC) $(CFLAGS) -c $(SOCKET_FILE)

$(MYLIB_O) : $(MYLIB_FILE)
	$(CC) $(CFLAGS) -DIPC -c $(MYLIB_FILE)

$(MYLIB): $(MYLIB_O) $(SOCKET_O)
	ld -shared -o $(MYLIB) $(MYLIB_O) $(SOCKET_O) -ldl
	@echo "Successfully Built $(MYLIB)"

$(SERVER): $(SERVER_FILE) $(SOCKET_O)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_FILE) $(SOCKET) $(LINK_PTHEAD)
	@echo "Successfully built $(SERVER)"

# format command
.PHONY: format
format:
	# clang-format all the source code files
	find *.c *.h | sed 's| |\\ |g' | xargs clang-format -style=google -i

# clean up command
.PHONY: clean
clean:
	# remove all the files with extension .o or .s or .so and executable
	rm -f $(SERVER) *.o *.s *.so