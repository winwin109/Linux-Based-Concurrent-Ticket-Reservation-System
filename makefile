all: server

server : server.cpp
	g++ -o server server.cpp -lpthread -ljsoncpp -lmysqlclient

clean:
	rm -rf server *.o
