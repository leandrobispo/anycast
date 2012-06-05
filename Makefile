all:
	gcc -g -c src/proxy.c
	gcc -g -c src/node.c
	gcc -g -c src/client.c
	gcc -g -c src/common.c
	gcc -g -o proxy proxy.o common.o
	gcc -g -o node node.o common.o
	gcc -g -o client client.o
clean:
	rm -rf *.o proxy node client
