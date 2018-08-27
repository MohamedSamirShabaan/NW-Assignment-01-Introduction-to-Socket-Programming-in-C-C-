# NW-Assignment-01-Introduction-to-Socket-Programming-in-C-C-
- implement client communicate with server using Persistent HTTP requests (TCP connection) and provide GET and POST requests .
- implement TXT , HTML , image , video(optional) in HTTP requests.


### How to run code using commands :
###### first define directory in client path called files and create text file called input.txt contains all requests :
- GET file_name host_name port_number
- Go to server path and type :
	g++ server.cpp -o s.out
	sudo ./s.out
- Go to client path and type :
	g++ client.cpp -o c.out
	./c.out localhost 80
- you can see requests and responses in terminal or can see in
each directory.



You can see code organization in pdf.
