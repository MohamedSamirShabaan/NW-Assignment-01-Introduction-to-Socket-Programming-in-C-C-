/*
** server.c -- a stream socket server demo
*/

#include <iostream>
#include <bits/stdc++.h>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>



using namespace std;


char * PORT =  "80" ; // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold


stringstream image(string path){
	std::ifstream InFile( path , std::ifstream::binary );
	std::vector<char> data( ( std::istreambuf_iterator<char>( InFile ) ), std::istreambuf_iterator<char>() );
	InFile.close();
   
	stringstream out;
	
	out << "HTTP/1.0 200 OK\\r\\n\n";

    for(int i = 0; i < (int) data.size() ; i++)
        {
                out << data[i];
        }
    return out;        
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_404(int new_fd){
	string response = "HTTP/1.0 404 Not Found\\r\\n\n";
	int len = response.size();
	char * response2 = &response[0];
	if (send(new_fd, response2, len, 0) == -1) {
		perror("sendall");
	}
}

string get_path(string file_name , string folder_name){
	char cwd[2000];
    getcwd(cwd, sizeof(cwd));
    string path;
    path = cwd + folder_name + file_name;
    cout << "Path:\n" << path << endl;
    return path;
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

void okay(int new_fd){
	string response = "OK";
	send(new_fd,response);
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

/* 
 * parse get request
 */
void parse_get(string request , int new_fd){
	string response = "";
	string path = "";
	int not_image = 0;
	string lines[6000];  //assume max file size = 6000 lines
	ifstream myReadFile;
	vector<string> tokens;
	
	// split on \r\n
	size_t pos = 0;
	string token;
	string delimiter = "\\r\\n";
	while ((pos = request.find(delimiter)) != string::npos) {
		token = request.substr(0, pos);
		cout << "token =" << token << endl;
		tokens.push_back(token);
		request.erase(0, pos + delimiter.length());
	}
	tokens.push_back(request);
	
	string line_file_name = tokens[0];

	// split on spaces
	vector<string> split_on_spaces = explode(line_file_name, ' ');

	//get file name
	string file_name = split_on_spaces[1].substr(1,split_on_spaces[1].size() - 1);;

	// find catogry	so split on .
	vector<string>  on_dots = explode(file_name , '.');
	   
	if (on_dots.size() < 2){
		response = "HTTP/1.0 404 Not Found\\r\\n\n";
		send(new_fd, response);
		return;
	}
	   
	string dir = on_dots[1];
	
	if (dir == "html"  || dir == "HTML"){
		not_image = 1;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/server/html/");
		myReadFile.open(path);
	 }else if (dir == "txt"  || dir == "TXT"){
		not_image = 1;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/server/txt/");
		myReadFile.open(path);
	 }else {
		// any type of image 
		not_image = 0;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/server/image/");
		myReadFile.open(path);
	}
	if (!myReadFile.is_open()){
		cout << "###################### File Not Found In Server Side! ######################\n";
		response = "HTTP/1.0 404 Not Found\\r\\n\n";
		send(new_fd, response);
		return;
	}else if (not_image == 1) {
		// read data from file
		response = "HTTP/1.0 200 OK\\r\\n\n";
		int i = 0;
		string str;
		while (getline(myReadFile, str)) {
			lines[i++] = str;
			cout << lines[i - 1] << endl;
		}
		myReadFile.close();

		int k = i;
		i = 0;
		while(i != k){
			response += lines[i++];
			response += "\n";
		}
		cout << "---------------------- send DATA to the client ----------------------\n";
		cout << "Response =\n" << response << endl;
		send(new_fd, response);
		return;
	}else {
		// read images
		myReadFile.close();
		stringstream ima = image(path);
		response = ima.str();
		cout << "---------------------- send Image to the client ----------------------\n";
		//cout << "Response image =\n" << response << endl;
		send(new_fd, response);
		return;
	}
}

/* 
 * parse post request
 */
void parse_post(string request , int new_fd){
	string path = "";
	string data = "";
	int not_image = 0;
	
	// split on \r\n
	vector<string> tokens;
	size_t pos = 0;
	string token;
	string delimiter = "\\r\\n";
	while ((pos = request.find(delimiter)) != string::npos) {
		token = request.substr(0, pos);
		tokens.push_back(token);
		request.erase(0, pos + delimiter.length());
	}
	tokens.push_back(request);
	
	string line_file_name = tokens[0];
	
	// split on spaces
	vector<string> split_on_spaces = explode(line_file_name, ' ');
	
	//get file name
	string file_name = split_on_spaces[1].substr(1,split_on_spaces[1].size() - 1);
	
	// find catogry	so split on "." dot
	vector<string>  on_dots = explode(file_name , '.');

	if(on_dots.size() < 2){ // No file extention
		send_404(new_fd);
		return;
	}
	   
	string dir = on_dots[1];
	
	// determine folder (extention)
	if (dir == "html"  || dir == "HTML"){
		not_image = 1;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/server/html/");
	}else if (dir == "txt"  || dir == "TXT"){
		not_image = 1;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/server/txt/");		
	}else {
		not_image = 0;
		string te = on_dots[0] +  "." +  on_dots[1];
		path = get_path( te , "/server/image/");  
	}
	
	// open file to write in it
	ofstream myWriteFile(path);
	if(path != "" && myWriteFile.is_open()){
		//send OK to the client
		cout << "---------------------- send Ok to the client ----------------------\n";
		string response = "OK";
		send(new_fd,response);
		
		//recv data
		data =  recv_timeout(new_fd, 4);
		
		// write data
		if (not_image == 1){
			cout << "RECVED DATA = \n" << data << endl;
			myWriteFile << data;
			myWriteFile.close();
			okay(new_fd);
		}else {
			ofstream outfile (path);
			outfile << data << endl;
			outfile.close();
			okay(new_fd);
		}	
	}else{
		send_404(new_fd);
		return;
	}
}

int get_new_fd(int sockfd){
	int new_fd;  //new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
	sin_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (new_fd == -1) {
		perror("accept");
		return -1;
	}

	inet_ntop(their_addr.ss_family,
	get_in_addr((struct sockaddr *)&their_addr),
		s, sizeof s);
	printf("---------------------- server: got connection from %s ----------------------\n", s);
	return new_fd;
}

int main(void)
{
    int sockfd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
	
	
    while(1) {	
		//accept comming request		
		 printf("---------------------- server: waiting for connections ----------------------\n");        
		int new_fd = get_new_fd(sockfd);
		while(new_fd == -1){new_fd = get_new_fd(sockfd);}
	    
		if (!fork()) {
			while(1){
				string request =  recv_timeout(new_fd, 4);
				printf("---------------------- server recv request from client ----------------------\n");
				
				if(request == ""){  //empty request
					close(new_fd);
					exit(0);
				}
				cout << "RECVED Request = \n" << request;
				
				// parse Get or Post
				if (request[0] == 'G' || request[0] == 'g'){ // parse Get
						parse_get(request , new_fd);
				}else if (request[0] == 'P' || request[0] == 'p'){ // parse Post
						parse_post(request , new_fd);
				}else {
					perror("###################### Error Request Method Name!! ######################\n");
				}
			}
		}
    }
    return 0;
}
