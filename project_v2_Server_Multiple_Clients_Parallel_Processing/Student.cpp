#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream> 

#define port_num "3418"
#define stud_1 "21518" //UDP ports for students 
#define stud_2 "21618"
#define stud_3 "21718"
#define stud_4 "21818"
#define stud_5 "21918"

class student {
private:
	float GPA;
	std::vector<std::string> interests;

	//TCP connection stuff
	//keep track of the server's info
	int sock;
	struct addrinfo server, *serverinfo, *p;
	int rv;

	//keep track of my own info
	struct sockaddr_in my_addr;
	char ip_addr[16];
	unsigned int my_port;
	socklen_t addr_size; 

	//extra data members for sending/recieving 
	char buffer[256]; //buffer for recieving messages
	std::vector<std::string> data;
	std::string my_stu;
	unsigned int filesize;

	//Phase 2 UDP data:
	int sockfd;
	struct addrinfo hints, *udp_info, *q;
	int sv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[256];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	std::string port_student; 

public:
	//methods 
	student(std::string stu_name);
	void loadfile(std::string filename);
	void reply();
	void print();
	void sendfile();
	void phase2(int stu_port);
};

student::student(std::string stu_name)
{

	my_stu = stu_name;

	//sets up the struct
	memset(&server, 0, sizeof(server));
	server.ai_family = AF_UNSPEC;
	server.ai_socktype = SOCK_STREAM;

	//grabs information of server & load into our class
	if(( rv = getaddrinfo("viterbi-scf2.usc.edu", port_num, &server, &serverinfo)) != 0)
	{
		std::cout << "Error getting info" << std::endl;
		exit(1);
	} 

	for(p = serverinfo; p != NULL; p = p->ai_next){
		if((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("Socket Error");
			continue;	
		}

		if(connect(sock, p->ai_addr, p->ai_addrlen) == -1){
			perror("Connect Error");
			close(sock);
			continue;
		}

		break;
	}

	if( p == NULL){
		std::cout << "Failed to connect" << std::endl;
		exit(2);
	}
	
	//find my own ip/port
	my_port = 0;
	while(my_port == 0){
		bzero(&my_addr, sizeof(addr_size));
		getsockname(sock, (struct sockaddr *) &my_addr, &addr_size);
		inet_ntop(AF_INET, &my_addr.sin_addr, ip_addr, sizeof(ip_addr));
		my_port = ntohs(my_addr.sin_port);
	}
	std::cout << my_stu << " has TCP port: " << my_port << " and IP address: " << ip_addr << std::endl;
	

	freeaddrinfo(serverinfo); //done with it - to prevent leaks
}

void student::loadfile(std::string filename)
{
	std::ifstream input(filename.c_str());
	std::string line; //used to parse the file
	
	std::getline(input, line); //first line - gpa
	std::stringstream ss(line);
	std::string temp;

	std::getline(ss, temp, ':');
	ss >> GPA; 

	while(std::getline(input, line))
	{	

		std::stringstream ss2(line);
		std::string temp2;
		std::getline(ss2, temp2, ':');
		std::string temp3;
		ss2 >> temp3;
		interests.push_back(temp3);
	}
}

void student::sendfile()
{	

	std::string application;
	std::ostringstream ss;
	ss << GPA;
	std::string temp(ss.str());
	application += my_stu + " " + temp;
	for(int i=0; i<interests.size(); i++)
	{
		application += " " + interests[i];
	}

	char char_array[application.length() + 1];
	strcpy(char_array, application.c_str());
	int len = strlen(char_array);
	send(sock, char_array, len, 0);
	
	std::cout << "Completed sending application for " << my_stu << std::endl;
}

void student::reply()
{

	int bytes_recv = recv(sock, buffer, 256, 0); //admission either sends 1 or 0
	std::string message(buffer);

	std::cout << my_stu << " has recieved the reply from the admission office" << std::endl;

	if(buffer[0] == '0') //student realizes that application info is wrong-terminates
	{
		std::cout << "Not Valid" << std::endl;
		close(sock);
		exit(1);
	}
	else
		std::cout << "Valid" << std::endl;
	
}

void student::phase2(int stu_port)
{

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	switch(stu_port)
	{
		case 1:
			port_student = stud_1;
			break;
		case 2:
			port_student = stud_2;
			break;
		case 3:
			port_student = stud_3;
			break;
		case 4:
			port_student = stud_4;
			break;
		case 5:
			port_student = stud_5;
			break;
		default:
			break;

	}
	char c_port_student[port_student.size()+1];
	strcpy(c_port_student, port_student.c_str());

	if((sv = getaddrinfo("viterbi-scf2.usc.edu", c_port_student, &hints, &udp_info)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	for(q = udp_info; q!= NULL; q=q->ai_next) {
		if((sockfd = socket(q->ai_family, q->ai_socktype, q->ai_protocol)) == -1){
			perror("listener: socket");
			continue;
		}

		if(bind(sockfd, q->ai_addr, q->ai_addrlen) == -1) {
			close(sockfd);
			perror("listner: bind");
			continue;
		}
		break;
	}

	if(q == NULL){
		fprintf(stderr, "listener: failed to create socket\n");
		return;
	}

	//find my own ip/port
	my_port = 0;
	while(my_port == 0){
		bzero(&my_addr, sizeof(addr_size));
		getsockname(sockfd, (struct sockaddr *) &my_addr, &addr_size);
		inet_ntop(AF_INET, &my_addr.sin_addr, ip_addr, sizeof(ip_addr));
		my_port = ntohs(my_addr.sin_port);
	}
	std::cout << my_stu << " has UDP port: " << my_port << " and IP address: " << ip_addr << "for Phase 2" << std::endl;
	freeaddrinfo(udp_info); //done with it - to prevent leaks

	//need only 1 recieve because either rejected or accepted to 1 program (priority)
	addr_len = sizeof(their_addr);
	if((numbytes = recvfrom(sockfd, buf, 256-1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1){
		perror("recvfrom");
		exit(1);
	}

	std::cout << my_stu << " has recieved the application result" << std::endl;
	std::cout << "End of phase 2 for " << my_stu << std::endl;
	
	close(sockfd);	
}

void student::print() //help debug code 
{
	std::cout << GPA << std::endl;

	for(int i=0; i<interests.size(); i++)
	{
		std::cout << interests[i] << std::endl;
	}
}

int main(int argc, char* argv[])
{

	std::string file = "student";

	for(int i=0;i<5;i++) // loop will run n times (n=5) 
    { 
        if(fork() == 0) 
        { 
        	std::stringstream ss;
        	ss << (i+1);
        	std::string temp = ss.str();
        	file += temp; 
            student my_student(file);
            file += ".txt";
            my_student.loadfile(file);
            my_student.sendfile();
            my_student.reply();
            my_student.phase2(i+1);
           	
            exit(0); 
        } 
    } 
	return 0;	
}