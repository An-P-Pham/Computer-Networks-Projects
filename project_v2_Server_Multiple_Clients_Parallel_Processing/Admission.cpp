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
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#define port_num "3418" //admissions TCP
#define dep_A "21218" //UDP ports for departments 
#define dep_B "21318"
#define dep_C "21418"
#define stud_1 "21518" //UDP ports for students 
#define stud_2 "21618"
#define stud_3 "21718"
#define stud_4 "21818"
#define stud_5 "21918"

//holds the student info
struct stu_info{
	std::string name;
	float GPA;
	std::vector<std::string> interests;
};

//holds the department info
struct dep_info{
	char name;
	std::map<std::string, float> programs;
};

//get sockaddr IPv6 or IPv4
void* get_in_addr(struct sockaddr *sa){
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool flag = true;

//does the actual sending
void send_UDP(char* udp_message, char* select_port, int len, std::string identifier)
{
	//set up UDP connections
	int sock_UDP;
	struct addrinfo hints, *servinfo, *q;
	int sv;
	int numbytes;

	//keep track of my own info
	struct sockaddr_in my_addr;
	char ip_addr[16];
	unsigned int my_port;
	socklen_t addr_size; 

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	if((sv = getaddrinfo("viterbi-scf2.usc.edu", select_port, &hints, &servinfo)) != 0){
				fprintf(stderr, "getaddrinfo %s\n", gai_strerror(sv));
				return;
	}

	for(q = servinfo; q!= NULL; q=q->ai_next) {
		if((sock_UDP = socket(q->ai_family, q->ai_socktype, q->ai_protocol)) == -1){
			perror("talker: socket");
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
		getsockname(sock_UDP, (struct sockaddr *) &my_addr, &addr_size);
		inet_ntop(AF_INET, &my_addr.sin_addr, ip_addr, sizeof(ip_addr));
		my_port = ntohs(my_addr.sin_port);
	}

	if(flag)
	{
		std::cout << "The admissions office has UDP port: " << my_port << " and IP address: " << ip_addr << "for Phase 2" << std::endl;
		flag = false; //print this message once
	}

	if((numbytes = sendto(sock_UDP, udp_message, len, 0, q->ai_addr, q->ai_addrlen)) == -1){
					perror("talker: sendto");
					exit(1);
	}
	if(identifier[0] == 's')
	{
		std::cout << "The admission office has send the application result to " << identifier << std::endl;
	}else
	{
		std::cout << "The admission office has send one admitted student to " << identifier << std::endl;
	}	
}

void UDP_phase(std::map<std::string, float> &depart_dataA, std::map<std::string, float> &depart_dataB, 
	std::map<std::string, float> &depart_dataC, std::map<std::string, stu_info> &student_map )
{
	
	//did student apply to right admissions?
	//process student info w/department
	//message format: Accept#A1#departmentA -> first to last interest
	//				  Reject

	bool reject;
	std::string results; //send to student 
	std::string acceptance; //send to department
	for(std::map<std::string, stu_info>::iterator it = student_map.begin(); it != student_map.end(); ++it)
	{	

		reject = true;

		for(int i=0; i < it->second.interests.size(); i++)
		{
			if(it->second.GPA >= depart_dataA[it->second.interests[i]] || 
				it->second.GPA >= depart_dataB[it->second.interests[i]] ||
				it->second.GPA >= depart_dataC[it->second.interests[i]])
			{
				reject = false;
				std::string temp(1, it->second.interests[i][0]); //department char to string
				results = "Accept#" + it->second.interests[i] + "#department" + temp; //message to send student

				std::string port_dep;
				//determine the department
				if(temp == "A")
					port_dep = dep_A;
				else if(temp == "B")
					port_dep = dep_B;
				else if(temp == "C")
					port_dep = dep_C;

				char c_port_dep[port_dep.size()+1];
				strcpy(c_port_dep, port_dep.c_str());

				std::string port_student;
				//determine the student to send to
				if(it->second.name == "student1")
					port_student = stud_1;
				else if(it->second.name == "student2")
					port_student = stud_2;
				else if(it->second.name == "student3")
					port_student = stud_3;
				else if(it->second.name == "student4")
					port_student = stud_4;
				else if(it->second.name == "student5")
					port_student = stud_5;
				
				char c_port_student[port_student.length()+1];
				strcpy(c_port_student, port_student.c_str());
				
				char char_array[results.length() + 1];
				strcpy(char_array, results.c_str());
				int len = strlen(char_array);

				if(reject)
				{
					//send to student only
					send_UDP(char_array, c_port_student, len, it->second.name);
				}
				else
				{
					//send to both
					std::string depart_name = "Department" + temp;
					std::stringstream ss;
					ss << it->second.GPA;
					std::string temp2 = ss.str();
					send_UDP(char_array, c_port_student, len, it->second.name);
					acceptance = it->second.name + "#" + temp2 + "#" + it->second.interests[i];
					char c_dep_message[acceptance.length()+1];
					strcpy(c_dep_message, acceptance.c_str());
					int len2 = strlen(c_dep_message);
					send_UDP(c_dep_message, c_port_dep, len2, depart_name);

				}
				

			}
		}	
	}

	std::cout << "End of Phase 2 for the admission office" << std::endl;

}

int main(void)
{
	struct addrinfo server, *serverinfo, *p; //server info
	struct sockaddr_in my_addr; 
	socklen_t addr_size;
	char ip_addr[16];
	unsigned int my_port;

	int rv;

	fd_set master; //master client list
	fd_set read_fds; //temp client list for select()
	int fdmax; //max # of clients we'll accept

	int listener; //current listening socket
	int newfd; //newly accepted client
	struct sockaddr_storage remoteaddr; //client's address
	socklen_t addrlen;

	char buffer[256]; //buffer for recieving messages 
	int nbytes; //message size	

	char remoteIP[INET6_ADDRSTRLEN];
	int yes = 1; //for socketopt() SO_REUSEADDR
	bool phase2 = false;

	std::map<std::string, float> depart_dataA;
	std::map<std::string, float> depart_dataB;
	std::map<std::string, float> depart_dataC;
	std::map<std::string, stu_info> student_map;
	//SET UP:

	//clear it 
	memset(&server, 0, sizeof(server));
	FD_ZERO(&master); //clear master and temp client sets
	FD_ZERO(&read_fds);

	//fill up struct data
	server.ai_family = AF_UNSPEC; //IPv4 vs IPv6
	server.ai_socktype = SOCK_STREAM; //TCP
	server.ai_flags = AI_PASSIVE;

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
		if((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("Socket Error");
			continue;	
		}

		//lose "address already in use" error
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if(bind(listener, p->ai_addr, p->ai_addrlen) == -1){
			close(listener);
			perror("Bind Error");
			continue;
		}

		break;
	}

	if( p == NULL){
		std::cout << "Failed to bind" << std::endl;
		exit(2);
	}
	
	//find my own ip/port - wait until we get a valid number
	my_port = 0;
	while(my_port == 0){
		bzero(&my_addr, sizeof(addr_size));
		getsockname(listener, (struct sockaddr *) &my_addr, &addr_size);
		inet_ntop(AF_INET, &my_addr.sin_addr, ip_addr, sizeof(ip_addr));
		my_port = ntohs(my_addr.sin_port);
	}
	std::cout << "The admission office has TCP port: " << my_port << " and IP address: " << ip_addr << std::endl;

	freeaddrinfo(serverinfo); //done with it - to prevent leaks

	//TCP
	if(listen(listener, 10) == -1) //listen to only 1 connection
	{
		perror("listening error");
		exit(3);
	}

	FD_SET(listener, &master); //add listener to master set

	fdmax = listener; //keep track of biggest descriptor -- only 1 so far

	for(;;){
		read_fds = master; //copy
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select error");
			exit(4);
		}

	//run through connections looking for data
	for(int i=0; i<= fdmax; i++){
		if(FD_ISSET(i, &read_fds)){ //there's data
			if(i == listener){
				//handle new connections
				addrlen = sizeof(remoteaddr);
				newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
				
				if(newfd == -1){
					perror("accept");
				}else{
					FD_SET(newfd, &master); //add to master set
					if(newfd > fdmax){
						fdmax = newfd; //keeps track of the max
					}
				}
			}else{
			

				if((nbytes = recv(i, buffer, sizeof(buffer), 0)) <= 0){
					close(i); 
					if((depart_dataA.size() == 3 && depart_dataB.size() == 3 && depart_dataC.size() == 3) && !phase2)
					{
						std::cout << "End of Phase 1 for the admissions office" << std::endl;
					}	

					FD_CLR(i, &master);
					if(student_map.size() == 5)
					{
						//UDP phase:
						
						UDP_phase(depart_dataA, depart_dataB, depart_dataC, student_map);
					}
				}else{

					if(buffer[0] != 's')
					{
						//data from department
						std::stringstream ss(buffer);
						std::string program;
						float GPA;
						if(buffer[0] == 'A'){ //department A
							getline(ss,program, '#');
							ss >> GPA;
							depart_dataA.insert(std::pair<std::string, float>(program, GPA));
							if(depart_dataA.size() == 3)
								std::cout << "Recieve the program list from Department A" << std::endl;
						}
						else if(buffer[0] == 'B'){ //department B
							getline(ss,program, '#');
							ss >> GPA;
							depart_dataB.insert(std::pair<std::string, float>(program, GPA));
							if(depart_dataB.size() == 3)
								std::cout << "Recieve the program list from Department B" << std::endl;
						}else{ //department C
							getline(ss,program, '#');
							ss >> GPA;
							depart_dataC.insert(std::pair<std::string, float>(program, GPA));
							if(depart_dataC.size() == 3)
								std::cout << "Recieve the program list from Department C" << std::endl;
						}
				
					}
					else
					{	

						if(!phase2){
							std::cout << "The admission office has TCP port: " << my_port << " and IP address: " << ip_addr << std::endl;
							phase2 = true;
						}
	
						std::stringstream ss(buffer);
						std::string application;
						stu_info stu_app;
						int j=0;
						while(std::getline(ss, application, ' '))
						{
							if(j == 0)//student name
							{
								stu_app.name = application;
							}else if(j == 1) //student's GPA
							{
								std::istringstream ss(application);
								ss >> stu_app.GPA;
							}else //interests 
							{
								stu_app.interests.push_back(application);
							}
							j++;
						}
						student_map.insert(std::pair<std::string, stu_info>(stu_app.name, stu_app));
						std::cout << "Admissions office recieved the application from " << stu_app.name << std::endl;
						bool exists = false;
						char reply[1];
						for(int k=0; k<stu_app.interests.size(); k++)
						{
							//apply to right programs?
							if(depart_dataA.find(stu_app.interests[k]) != depart_dataA.end() || 
								depart_dataB.find(stu_app.interests[k]) != depart_dataB.end() ||
								depart_dataC.find(stu_app.interests[k]) != depart_dataC.end())
								{
									exists = true;
								}
						} 

						//handles the acknowledgement
						if(exists == false)	
							reply[0] = '0';
						else
							reply[0] = '1';

						int len = strlen(reply);
						send(i, reply, len, 0);
						//stu_map.insert(std::pair<std::string, stu_info>(stu_app.name, stu_app)); //creating the data base
					}
				}
			}
		}
	}
}

	
	return 0;
}