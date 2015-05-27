.PHONY: all

all: server
	./server

server: server.cc
	@g++ -g -pthread -std=c++0x  -o server server.cc
