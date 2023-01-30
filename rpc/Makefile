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
LINK_DIRTREE = -ldirtree

# the target files and library for the project
SOCKET_FILE = socket.c
SOCKET_O = socket.o

MARSHALL_FILE = marshall.c
MARSHALL_O = marshall.o

MYLIB_FILE = mylib.c
MYLIB_O = mylib.o
MYLIB = mylib.so

SERVER_FILE = server.c
SERVER_O = server.o
SERVER = server

# threading needed for server to serve concurrent requests
LINK_PTHREAD   = -lpthread

all: $(MYLIB) $(SERVER)
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH;$PWD/lib;"

$(MARSHALL_O): $(MARSHALL_FILE)
	$(CC) $(CFLAGS) -c $(MARSHALL_FILE) $(INCLUDE_PATH)

$(SOCKET_O) : $(SOCKET_FILE)
	$(CC) $(CFLAGS) -c $(SOCKET_FILE)

$(MYLIB_O) : $(MYLIB_FILE)
	$(CC) $(CFLAGS) -DIPC -c $(MYLIB_FILE) $(INCLUDE_PATH)

$(MYLIB): $(MYLIB_O) $(SOCKET_O) $(MARSHALL_O)
	ld -shared -o $(MYLIB) $(MYLIB_O) $(SOCKET_O) $(MARSHALL_O) $(LIBRARY_PATH) ${LINK_DIRTREE}
	@echo "Successfully Built $(MYLIB)"

$(SERVER_O): $(SERVER_FILE)
	$(CC) -c $(SERVER_FILE) $(INCLUDE_PATH)

$(SERVER): $(SERVER_O) $(SOCKET_O) $(MARSHALL_O)
	$(CC) -o $(SERVER) $(SERVER_O) $(SOCKET_O) $(MARSHALL_O) $(LINK_PTHEAD) $(LIBRARY_PATH) ${LINK_DIRTREE}
	@echo "Successfully built $(SERVER)"

# format command
.PHONY: format
format:
	# clang-format all the source code files
	find *.c *.h | sed 's| |\\ |g' | xargs clang-format -style=google -i

# submission helper command
.PHONY: submit
submit:
	# submit by compressing tar
	tar cvzf ../mysolution.tgz design.pdf Makefile *.h *.c

# clean up command
.PHONY: clean
clean:
	# remove all the files with extension .o or .s or .so and executable with tar
	rm -f $(SERVER) *.o *.s *.so ../mysolution.tgz
