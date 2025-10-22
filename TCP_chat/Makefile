all: server client menu
	sudo ./server & 	./menu
chat_server: server.c
	gcc -Wall -Wextra -O2 -o server server.c

chat_client: client.c
	gcc -Wall -Wextra -O2 -o client client.c

menu: menu.c
	gcc -Wall -Wextra -O2 -o menu menu.c


clean:
	rm -f server client menu

