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
#include <fstream>
#include <sstream>
#include <iostream> 
#include <vector>

#define port_num "3418" //admissions TCP
#define dep_A "21218" //UDP ports for departments 
#define dep_B "21318"
#define dep_C "21418"

class department{
private:
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
	std::string my_dep;
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
	std::string port_dep;

public:
	department(std::string dep_name);
	~department();
	void loadfile(std::string filename);
	void sendfile();
	void printdata();
	void phase2(char dep_ID);
	int sendall(int s, char* buf, int* len);
};

department::department(std::string dep_name)
{
	my_dep = dep_name;

	//sets up the struct
	memset(&server, 0, sizeof(server));
	server.ai_family = AF_UNSPEC;
	server.ai_socktype = SOCK_STREAM;

	//grabs information of server & load into our class
	if(( rv = getaddrinfo("127.0.0.1", port_num, &server, &serverinfo)) != 0)
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
	std::cout << my_dep << " has TCP port: " << my_port << " and IP address: " << ip_addr << std::endl;
	std::cout << my_dep << " is now connected to the admission office" << std::endl;

	freeaddrinfo(serverinfo); //done with it - to prevent leaks
}

department::~department()
{
	close(sock);
}

void department::loadfile(std::string filename)
{
	filesize = 0;
	std::ifstream input(filename.c_str());
	std::string line; //used to parse the file
	while(std::getline(input, line))
	{
		char dep_info[line.length() + 1];
		strcpy(dep_info, line.c_str());
		data.push_back(dep_info);
		filesize++;
	}
}

int department::sendall(int s, char* buf, int* len)
{
	int total = 0;
	int bytesleft = *len;
	int n;

	while(total < *len){
		n = send(s, buf+total, bytesleft, 0);
		if(n == -1) {break;}
		total == n;
		bytesleft -= n;
	}

	*len = total; //return number actually sent

	return n==-1? -1:0; //-1 on failure and 0 on success
}

void department::sendfile()
{
	
	int i=0;
	while(i != filesize)
	{
		char char_array[data[i].length() + 1];
		strcpy(char_array, data[i].c_str());
		int len = strlen(char_array);
		send(sock, char_array, len, 0); //re-try until u can send it
		std::cout << my_dep << " has sent " << char_array << " to the admission office" << std::endl;
		i++;
		
	}

	std::cout << "Updating the admission office is done for " << my_dep << std::endl;
	std::cout << "End of Phase 1 for " << my_dep << std::endl;
	close(sock); 
	
}

void department::phase2(char dep_ID)
{
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	switch(dep_ID)
	{
		case 'A':
			port_dep = dep_A;
			break;
		case 'B':
			port_dep = dep_B;
			break;
		case 'C':
			port_dep = dep_C;
			break;
		default:
			break;

	}
	char c_port_dep[port_dep.size()+1];
	strcpy(c_port_dep, port_dep.c_str());

	if((sv = getaddrinfo("viterbi-scf2", c_port_dep, &hints, &udp_info)) != 0){
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
	std::cout << my_dep << " has UDP port: " << my_port << " and IP address: " << ip_addr << " for Phase 2" << std::endl;
	freeaddrinfo(udp_info); //done with it - to prevent leaks

	addr_len = sizeof(their_addr);

	//can recived acceptances from multiple students 
	while(1){

		if((numbytes = recvfrom(sockfd, buf, 256-1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}

		std::cout << buf << "has been admitted to " << my_dep << std::endl;
	}

	std::cout << "End of Phase 2 for " << my_dep << std::endl;

	close(sockfd);	 
}

int main(int argc, char* argv[])
{

	std::string filename;
	std::vector<department> total_departments;
	char dep_ID;
	int n1 = fork(); //creates first child

	int n2 = fork(); //creates 2nd child and grandchild

	if(n1 > 0 && n2 > 0){
		//parent have it do nothing

	}
	else if(n1 == 0 && n2 > 0) //first child
	{
		
		filename = "departmentA";
		dep_ID = filename[10];
		department depA(filename);
		filename += ".txt";
		depA.loadfile(filename);
		depA.sendfile();
		//depA.phase2(dep_ID);
	}
	else if(n1 > 0 && n2 == 0) //second child
	{
		filename = "departmentB";
		dep_ID = filename[10];
		department depB(filename);
		filename += ".txt";
		depB.loadfile(filename);
		depB.sendfile();
		//depB.phase2(dep_ID);
	}
	else //third child 
	{
		
		filename = "departmentC";
		dep_ID = filename[10];
		department depC(filename);
		filename += ".txt";
		depC.loadfile(filename);
		depC.sendfile();
		//depC.phase2(dep_ID);
	}

	return 0;
}