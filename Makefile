all:
	gcc *.c -lcurl -lssl -lcrypto -lpq -lpthread -liniparser -g3
