all: server client serviceMap

server: server_tcp.c
	gcc -o server server_tcp.c

client: client.go
	go build client.go

serviceMap: serviceMap.go
	go build serviceMap.go

clean:
	rm -f server client serviceMap