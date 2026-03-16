CC = gcc
CFLAGS = -O2 -Wall -Wextra

SERVER = p1-dataProgram
GUI_BIN = p1-gui

SERVER_SRC = p1-dataProgram.c
GUI_SRC = p1-gui.c

.PHONY: all server gui run clean stop demo

all: $(SERVER) $(GUI_BIN)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER)

$(GUI_BIN): $(GUI_SRC)
	$(CC) $(CFLAGS) $(GUI_SRC) -o $(GUI_BIN)

server: $(SERVER)
	./$(SERVER)

gui: $(GUI_BIN)
	./$(GUI_BIN)

run: all
	@echo "Abrir dos terminales: 'make server' y 'make gui'"

stop:
	-pkill -f './$(SERVER)'

demo: all
	@echo "Demo automatica: busqueda por genero hip hop"
	@./$(SERVER) >/tmp/p1_server_demo.log 2>&1 & \
	server_pid=$$!; \
	sleep 2; \
	printf '2\nhip hop\n3\n' | ./$(GUI_BIN); \
	kill $$server_pid >/dev/null 2>&1 || true

clean:
	rm -f $(SERVER) $(GUI_BIN) gui