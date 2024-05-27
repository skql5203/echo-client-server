all: echo-client echo-server

echo-client: echo-client.cpp
	g++ -o $@ $<

echo-server: echo-server.cpp
	g++ -pthread -o $@ $<

clean:
	rm -f echo-client echo-server

