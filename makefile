server: http_conn.o main.o 
	g++ http_conn.o main.o -o server -lpthread
http_conn.o: http_conn.cpp
	g++ -c http_conn.cpp -o http_conn.o -g -Wall
main.o: main.cpp
	g++ -c main.cpp -o main.o -g -Wall
clean:
	rm main.o http_conn.o server