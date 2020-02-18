CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 1234

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1
ID_CLIENT = "numee"

all: server subscriber

# Compileaza server.c
server: server.cpp

# Compileaza subscriber.c
subscriber: subscriber.cpp

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
