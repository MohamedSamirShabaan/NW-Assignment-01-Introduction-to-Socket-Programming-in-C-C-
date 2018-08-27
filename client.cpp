/*
** client.c -- a stream socket client demo
*/
#include <iostream>
#include <bits/stdc++.h>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>

using namespace std;


char* port  = "80"; // the port client will be connecting to 
char* host = "localhost";
string version = "HTTP/1.0";
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

string get_path(string file_name , string folder_name){
	char cwd[2000];
    getcwd(cwd, sizeof(cwd));
    string path;
    path = cwd + folder_name + file_name;
    cout << "Path:\n" << path << endl;
    return path;
}

int sendall(int sockfd, char* buf, int *len){
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(sockfd, buf + total ,  bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
        cout << n << endl;
    }
    *len = total; // return number actually sent here
    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

void send(int sockfd, string data){
	int length2 = data.size();
	char * body2 = &data[0];
	if (sendall(sockfd , body2 , &length2) == -1) {
		perror("###################### sendall ######################\n");
		printf("###################### We only sent %d bytes because of the error!  ######################\n", length2);
	}
}

const vector<string> explode(const string& s, const char& c){
	string buff{""};
	vector<string> v;
	for(auto n:s){
		if(n != c) buff+=n; else
		if(n == c && buff != "") { v.push_back(buff); buff = ""; }
	}
	if(buff != "") v.push_back(buff);
	return v;
}

string recv_timeout(int sockfd , int timeout){
    int size_recv , total_size= 0;
    struct timeval begin , now;
    char chunk[512];
    double timediff;
     
    //non blocking socket
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    
    //START TIMES
    gettimeofday(&begin , NULL);
    string result = "";
    while(1){ 
		// update time
        gettimeofday(&now , NULL);
        timediff = (now.tv_sec - begin.tv_sec) + 1e-6 * (now.tv_usec - begin.tv_usec);
         
		// getting data and timeout occure --> finish
        if(total_size > 0 && timediff > timeout){break;}
         
        //still trying t get data until 3 times timeout
        else if( timediff > timeout*3){break;}
         
        // still recv data in chunks and append all together
        memset(chunk ,0 , 512);
        while ((size_recv =  recv(sockfd , chunk , 512 , 0) ) > 0){
            result.append(chunk, size_recv);
            total_size += size_recv;
            //statr new time (reset the counter)
            gettimeofday(&begin , NULL);
        }
    }
    return result;
}

stringstream image(string path){
	std::ifstream InFile( path , std::ifstream::binary );
	std::vector<char> data( ( std::istreambuf_iterator<char>( InFile ) ), std::istreambuf_iterator<char>() );
	InFile.close();
	stringstream out;
    for(int i = 0; i < (int) data.size() ; i++){out << data[i];}
    return out;        
}

/*
 * open file and get data from it in post request
 */
string open_file_get_data(string file_name){
	string data = "" ;
	ifstream myReadFile;
	
	vector<string>  on_dots = explode(file_name , '.');
	
	if (on_dots.size() < 2){ //assume all files should have extention
			return "-2";
	}
	string dir = on_dots[1];
	int not_image = 0;
	string path = "";	
	if(dir == "html"  || dir == "HTML"){
		not_image = 1;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/client/html/");	
		myReadFile.open(path);
	}else if (dir == "txt"  || dir == "TXT"){
		not_image = 1;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/client/txt/");
		myReadFile.open(path);
	}else { // any type of image or any other thing (i.e. video)
		not_image = 0;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/client/image/");
		myReadFile.open(path);
	}
	if(!myReadFile.is_open()){
		return "-1";
	}else if (not_image == 1){
		// read data from file Assume txt and html
		string str;
		while (getline(myReadFile, str)) {
			data += str;
			data += "\n";
		}
		myReadFile.close();
		return data;
	}else {
		myReadFile.close();
		stringstream ima = image(path);
		data = ima.str();
		return data;
	}
}

/*
 * parse reponse in get and get data from it if 200 and create file in certain directory
 */
 
void store_to_dirctory(string response , string file_name){
	// parse response to know if 200 or 404
	
	// split on \r\n
	vector<string> tokens;
	size_t pos = 0;
	string token;
	string delimiter = "\\r\\n";
	while ((pos = response.find(delimiter)) != string::npos) {
		token = response.substr(0, pos);
		tokens.push_back(token);
		response.erase(0, pos + delimiter.length());
	}
	tokens.push_back(response);
	
	// split on spaces
	string line_of_type = tokens[0];
	vector<string> split_on_spaces = explode(line_of_type, ' ');
	string type = split_on_spaces[1];
		
	// 404 or 200
	if (type == "404"){
			cout << "status code of response is 404, can't store any files!!\n";
			return ;
	}else { //200
		string data = tokens[1];
		vector<string>  on_dots = explode(file_name , '.');
		if (on_dots.size() < 2){
			cout << "Error No File Extension!!\n";
			return;
		}
		string dir = on_dots[1];
		string path;
		int not_image = 0;
		if (dir == "html"  || dir == "HTML"){
			not_image = 1;
			string te = on_dots[0] +  "." +  on_dots[1];
			path = get_path( te , "/client/html/");	
		}else if (dir == "txt"  || dir == "TXT"){
		 	not_image = 1;
		 	string te = on_dots[0] +  "." +  on_dots[1];
		 	path = get_path( te , "/client/txt/");
		}else { // any type of image or any other thing (i.e. video)
			not_image = 0;
			string te = on_dots[0] +  "." +  on_dots[1];
			path = get_path( te , "/client/image/");  
		}
		if (not_image == 1){
			ofstream outfile (path);
			outfile << data.substr(1 , data.size() - 1) << endl;
			outfile.close();
			cout << "---------------------- SAVED IN " << dir << " ----------------------\n";
		}else {
			ofstream outfile (path);
			outfile << data.substr(1 , data.size() - 1) << endl;
			outfile.close();
			cout << "---------------------- SAVED IN " << dir << " ----------------------\n";
		}
	}
}

int open_connection(){
	int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	// Get Address Infortmation
	if ((rv = getaddrinfo(host, port , &hints, &servinfo)) != 0) {
		fprintf(stderr, "##################### getaddrinfo: %s #####################\n", gai_strerror(rv));
		return -1;
	}
	
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("##################### client: socket #####################\n");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("##################### client: connect #####################\n");
			continue;
		}
		break;
	}
	
	// means i reah to the end of the list without find a good connection
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}
	
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
				s, sizeof s);
	printf("---------------------- client: connecting to %s ----------------------\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	return sockfd;
}

int main(int argc, char *argv[])
{
	// declaration
    string connection = "Connection: Keep-Alive\\r\\n\n";
	string accept = "Accept: text/html , text/plain , image/*\\r\\n\n";
	string hostName = "Host: ";
	string endOfLine = "\\r\\n\n";
	ifstream myReadFile;
	int i = 0;
    string str;
	
	// getting host and port from user
    if (argc != 3) {
        fprintf(stderr,"usage: client hostname or port number >> input doesn't enough\n enter something like: GET test.html localhost 80\n");
        exit(1);
    }else if(argc == 3){
		host = argv[1];
		port = argv[2];
	}
	
	// parsing the input file
    string lines[100]; // assume max #requests in 100
    string path = get_path("input.txt" , "/files/");
    myReadFile.open(path);
    if (myReadFile.is_open()) {
		while (getline(myReadFile, str)) {
			lines[i++] = str;
		}
	}else {
		cout << "##################### Error in parsing input file #####################\n";
		exit(0);	
	}    
    myReadFile.close();
    
    // open connection with the server
    int sockfd = open_connection();
    if(sockfd == -1){ //failed to open connection
		perror("##################### failed to open connection #####################\n");
		exit(1);
	}
    
    cout << "---------------------- NEW REQUEST ----------------------\n";
    //for each line --> make request
    int k = i;
    i = 0;
    while(i != k){ //main loop
		// get command line
		string line = lines[i++];
		cout << "Line = " << line << endl;
		if(line == ""){continue;}
		
		//split line on spaces
		vector<string> tokens = explode(line, ' ');
		
		//making request
		string request = "";
		string file_name = tokens[1];
		request = tokens[0] + " /" + tokens[1] + " " + version + endOfLine + hostName + tokens[2] + endOfLine + connection + accept + endOfLine;
		
		// GET or POST
		if (tokens[0] == "GET" || tokens[0] == "get"){ //GET
			// send get request to server
			cout << "---------------------- SEND GET REQUEST TO SERVER ----------------------\n";			
			send(sockfd, request);
			cout << "request = \n" << request << endl;
			
			//recive the response from the server
			printf("---------------------- CLIENT RECV RESPONSE FROM SERVER ----------------------\n");
			string res = recv_timeout(sockfd , 4);
			cout << "Recieved Response =  \n" << res << endl;
			
			// store data in file with same name and in extension dir
			store_to_dirctory(res , file_name);			
		}else if (tokens[0] == "POST" || tokens[0] == "post"){ //POST
			//check if file exists in client side
			string body = open_file_get_data(file_name);
			if(body == "-1"){
				perror("###################### File Not Found In Client Side!! ######################\n");
			}else if(body == "-2"){
				perror("###################### Error No File Extension!! ######################\n");
			}else{
				// send post request to server
				cout << "---------------------- SEND POST REQUEST TO SERVER ----------------------\n";
				send(sockfd, request);
				cout << "request = \n" << request << endl;
				
				//recive the response OK from the server
				printf("---------------------- CLIENT RECV OK FROM SERVER ----------------------\n");
				string res = recv_timeout(sockfd , 4);
				cout << " RECIEVED OK = " << res << endl;
				if (res == "OK"){
					printf("---------------------- CLIENT SEND DATA TO SERVER ----------------------\n");
					send(sockfd, body);
					recv_timeout(sockfd , 4);
				}else if(res == "HTTP/1.0 404 Not Found\\r\\n\n"){
					perror("###################### HTTP/1.0 404 Not Found\\r\\n    can't find path on server side ######################\n");
				}else {
					perror("###################### Must recv OK response from server\n ######################");
				}
			}
		}else { //not get nor post
			 perror("###################### Undefined Method Only Support GET and POST Methods\n ######################");
		}
		cout << "---------------------- END REQUEST ----------------------\n";
		if(i != k)cout << "---------------------- NEW REQUEST ----------------------\n";
	}
	close(sockfd);
    return 0;
}
