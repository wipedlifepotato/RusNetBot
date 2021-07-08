all:
	gcc *.c -lcurl -lssl -lcrypto -lpq -g -o a.out -lpthread
