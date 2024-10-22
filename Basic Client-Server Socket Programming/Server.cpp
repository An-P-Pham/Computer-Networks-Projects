#include<iostream>
#include<string>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h> //for srand & rand
#include<time.h> //time
#include<bits/stdc++.h> //string to char* 
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/wait.h>
#include<sstream>

class tcp_server
{
private:
	int sock;
	int port_num;
	struct addrinfo server, *serverinfo, *p; //server info
	struct sockaddr client_addr; //client's info
	socklen_t addr_size;  
	int rv; //not sure what this is for yet
	int client_socket;
	char buffer[1024]; //buffer for recieving messages 
	std::string ip_addr; //for the client

public:
	tcp_server(); //constructor
	~tcp_server(); //destructor closes port after finishing program 
	bool conn(); //connect to client

	//phase functions
	void offer();
	void ack();

};

//constructor sets up socket, binds, listens and accepts - useful for only 1 socket
tcp_server::tcp_server()
{	
	//clear it 
	memset(&server, 0, sizeof(server));

	//fill up struct data
	server.ai_family = AF_UNSPEC; //IPv4 vs IPv6
	server.ai_socktype = SOCK_STREAM; //TCP
	server.ai_flags = AI_PASSIVE;

	//set static port number
	port_num = 3300+118;  //based on USC ID # 
	std::stringstream ss;
	ss << port_num;
	std::string temp = ss.str();
	char char_array[temp.length() + 1];
	strcpy(char_array, temp.c_str());

	//grabs information of server & load into our class
	//viterbi-scf2.usc.edu
	//127.0.0.1
	if(( rv = getaddrinfo("viterbi-scf2.usc.edu", char_array, &server, &serverinfo)) != 0)
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

		if(bind(sock, p->ai_addr, p->ai_addrlen) == -1){
			close(sock);
			perror("Bind Error");
			continue;
		}

		break;
	}

	if( p == NULL){
		std::cout << "Failed to bind" << std::endl;
		exit(2);
	}


	freeaddrinfo(serverinfo); //done with it - to prevent leaks

}

tcp_server::~tcp_server()
{
	close(sock);
}

bool tcp_server::conn()
{
	if(listen(sock, 5) == -1) //listen to only 1 connection
	{
		perror("listening error");
		exit(EXIT_FAILURE);
	}

	addr_size = sizeof(client_addr);
	if((client_socket = accept(sock, (struct sockaddr *)&client_addr, &addr_size)) == -1 )
	{
		perror("accept error");
		exit(EXIT_FAILURE);
	}
}

void tcp_server::offer()
{
	//reads transaction ID
	int bytes_recv = recv(client_socket, buffer, 1024, 0);
	std::cout << "Discovery: " << std::endl;
	std::cout << "Transaction ID: " << buffer << std::endl;

	//generates IPv4/TransID & sends
	//std::string message = std::to_string(rand()%255) + " ";
	std::stringstream ss;
	ss << rand()%255;
	ss << " ";
	std::string message = ss.str();
	ss.str(""); //clears it 
	for(int i=0; i<3; i++)
	{	
		ss << rand()%255;
		ip_addr += ss.str() + ".";
		ss.str("");
	}
	std::string temp2(buffer);
	ip_addr += temp2;
	message += ip_addr;
	std::cout << "New IP: " << ip_addr << std::endl << std::endl;
	char char_array[message.length() + 1];
	strcpy(char_array, message.c_str());
	int len = strlen(char_array);
	int byte_size = send(client_socket, char_array, len, 0); //sends transaction data
}

void tcp_server::ack()
{
	//reads transaction ID
	std::cout << "Request: " << std::endl;
	int bytes_recv = recv(client_socket, buffer, 1024, 0);

	std::string message(buffer);
	std::stringstream ss(message);
	std::string temp;
	int i = 0;
	while(std::getline(ss, temp, ' ')){
		switch(i)
		{
			case 0:
				std::cout << "Transaction ID: " << temp << std::endl;
				break;
			case 1:
				std::cout << "Client Echo IP: " << temp << std::endl;
				break;
			default:
				break;
		}
		i++;
	}


	//echos IP & transaction ID
	std::stringstream ss2;
	ss2 << rand()%255;
	std::string temp2 = ss2.str();
	temp2 += " ";
	temp2 += ip_addr;

	char char_array[temp2.length() + 1];
	strcpy(char_array, temp2.c_str());
	int len = strlen(char_array);
	int byte_size = send(client_socket, char_array, len, 0);	
}

int main(int argc, char* argv[])
{	

	srand(time(0));

	//set up
	tcp_server DHCP;
	DHCP.conn();

	//Offer Phase
	DHCP.offer();
	DHCP.ack();
	
	return 0;
}
