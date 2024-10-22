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
#include<sstream>
#include<errno.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/wait.h>

class tcp_client
{
private:
	int sock;
	int port_num;
	struct addrinfo server, *serverinfo, *p;
	int rv;
	char buffer[1024]; //buffer for recieving messages 

public:
	tcp_client(); //constructor
	~tcp_client(); 
	void discovery(); //generates 8-bit number for transID and sends to server
	void request(); //echos generated IP address & sends a randomly generated transaction ID
	void ack(); //echos the acknowledgement, transID, and IP


};

//constructor creates socket and connects to server
tcp_client::tcp_client()
{	
	//sets up the struct
	memset(&server, 0, sizeof(server));
	server.ai_family = AF_UNSPEC;
	server.ai_socktype = SOCK_STREAM;

	//set static port number
	port_num = 3300+118;  //based on USC ID # 
	std::stringstream ss;
	ss << port_num;
	std::string temp = ss.str();
	char char_array[temp.length() + 1];
	strcpy(char_array, temp.c_str());

	//grabs information of server & load into our class
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

	freeaddrinfo(serverinfo); //done with it - to prevent leaks
}

tcp_client::~tcp_client()
{
	close(sock);
}

void tcp_client::discovery()
{
	//create the transaction ID
	std::stringstream ss;
	ss << rand() % 255; //give a random number 0-255
	std::string temp = ss.str();
	char char_array[temp.length() + 1];
	strcpy(char_array, temp.c_str());
	int len = strlen(char_array);
	int byte_size = send(sock, char_array, len, 0); //sends transaction data
}

void tcp_client::request()
{
	//echos recieved address
	std::cout << "Offer: " << std::endl;
	int bytes_recv = recv(sock, buffer, 1024, 0);
	std::string message(buffer);
	std::stringstream ss(message);
	std::string temp;
	std::string temp2;
	int i = 0;
	while(std::getline(ss, temp, ' ')){
		switch(i)
		{
			case 0:
				std::cout << "Transaction ID: " << temp << std::endl;
				break;
			case 1:
				std::cout << "Recieved IP Address: " << temp << std::endl << std::endl;
				temp2 = temp;
				break;
			default:
				break;
		}
		i++;
	}

	//sends another transID
	std::stringstream ss2;
	ss2 << rand()%255;
	temp = ss2.str();
	temp += " " + temp2;
	char char_array[temp.length()+1];
	strcpy(char_array, temp.c_str());
	int len = strlen(char_array);
	int byte_size = send(sock, char_array, len, 0);
}

void tcp_client::ack()
{	
	std::cout << "ACK: " << std::endl;
	int bytes_recv = recv(sock, buffer, 1024, 0);
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
				std::cout << "Acknowledged IP Address: " << temp << std::endl;
				break;
			default:
				break;
		}
		i++;
	}
}

int main(int argc, char* argv[])
{
	srand(time(0));
	tcp_client myClient;
	myClient.discovery();
	myClient.request();
	myClient.ack();
	
	return 0;
}
