synacor: main.c
	gcc -std=c89 -Wall -Wextra -pedantic -fextended-identifiers main.c -static-libgcc -o synacor.exe
debug: main.c
	gcc -DDEBUG -std=c89 -Wall -Wextra -pedantic -fextended-identifiers main.c -static-libgcc -o synacor.exe
