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
#include <string>
#include <vector>
#include <signal.h>
//#include <sys/mman.h>

#define port_num "3418" //admissions TCP
#define dep_A "21218" //UDP ports for departments 
#define dep_B "21318"
#define dep_C "21418"
#define stud_1 "21518" //UDP ports for students 
#define stud_2 "21618"
#define stud_3 "21718"
#define stud_4 "21818"
#define stud_5 "21918"

//get sockaddr IPv6 or IPv4
void* get_in_addr(struct sockaddr *sa){
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s){
	//waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

//holds the department info
struct dep_info{
	char name;
	std::map<std::string, float> programs;
};

//holds the student info
struct stu_info{
	std::string name;
	float GPA;
	std::vector<std::string> interests;
};

// void student(int sock, stu_info stu_app)
// {
// 	//did student apply to right admissions?
// 	//process student info w/department
// 	//message format: Accept#A1#departmentA -> first to last interest
// 	//				  Reject
// 	bool exists = false;
// 	bool reject = true;
// 	std::string results;
// 	for(int i=0; i<stu_app.interests.size(); i++)
// 	{
// 		//apply to right programs?
// 		if(depart_dataA.find(stu_app.interests[i]) != depart_dataA.end() || 
// 			depart_dataB.find(stu_app.interests[i]) != depart_dataB.end() ||
// 			depart_dataC.find(stu_app.interests[i]) != depart_dataC.end())
// 		{
// 			exists = true;
// 			if(stu_app.GPA >= depart_dataA[stu_app.interests[i]] || 
// 				stu_app.GPA >= depart_dataB[stu_app.interests[i]] ||
// 				stu_app.GPA >= depart_dataC[stu_app.interests[i]])
// 			{
// 				reject = false;
// 				std::string temp(1, stu_app.interests[i][0]); //department char to string
// 				results = "Accept#" + stu_app.interests[i] + "#department" + temp;
// 			}
// 		}
// 	} 

// 	//handles the acknowledgement
// 	if(exists == false)	
// 		results = "0";
// 	else
// 		results = "1";

// 	char char_array[results.length() + 1];
// 	strcpy(char_array, results.c_str());
// 	int len = strlen(char_array);
// 	send(sock, char_array, len, 0);

// 	// else
// 	// {
// 	// 	if(reject)
// 	// 	{
// 	// 		results = "Reject";
// 	// 		char char_array[results.length() + 1];
// 	// 		strcpy(char_array, results.c_str());
// 	// 		int len = strlen(char_array);
// 	// 		send(sock, char_array, len, 0);
// 	// 	}
// 	// 	else
// 	// 	{
// 	// 		char char_array[results.length() + 1];
// 	// 		strcpy(char_array, results.c_str());
// 	// 		int len = strlen(char_array);
// 	// 		send(sock, char_array, len, 0);
// 	// 	}
// 	// }
// }

int main(void)
{
	//holds the server data
	struct addrinfo server, *serverinfo, *p; //server info
	struct sockaddr_in my_addr; 
	socklen_t addr_size;
	char ip_addr[16];
	unsigned int my_port;
	int rv;

	//deals with dead processes
	socklen_t sin_size;
	struct sigaction sa;
	char s[INET6_ADDRSTRLEN];

	//listen and accepting new 
	int listener; //current listening socket
	int newfd; //newly accepted client
	struct sockaddr_storage remoteaddr; //client's address
	socklen_t addrlen;
	char buffer[256]; //buffer for recieving messages 
	int nbytes; //message size	
	int yes = 1; //for socketopt() SO_REUSEADDR

	//data to use and process
	int total_con;
	char *phase2;

	int pipefd[2];
	pipe(pipefd);
	char pipe_buf;
	std::map<std::string, dep_info> depart_data;
	std::map<std::string, stu_info> stu_map;

	//SET UP:
	memset(&server, 0, sizeof(server));

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
	if(( rv = getaddrinfo("127.0.0.1", char_array, &server, &serverinfo)) != 0)
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
		if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			perror("setsockopt");
			exit(1);
		}

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

	//The Main Process:
	int pid;
	if(listen(listener, 10) == -1) //listen to only 1 connection
	{
		perror("listening error");
		exit(3);
	}
	//handles zombie processes
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1){
		perror("sigaction");
		exit(1);
	}
	//always listening for new connections 
	while(1){  

		//handle new connections
 		addrlen = sizeof(remoteaddr);
		newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
		if( newfd == -1 ) 
		{
			//perror("accept error");
			continue;
		}
		total_con++; //keep track for phase 2
		pid = fork();

		if(pid > 0) //parent
		{
			close(newfd); //parent doesnt need child
			// close(pipefd[1]);
			// while( read(pipefd[0], &pipe_buf, 1) > 0)
			// 	write(1, &pipe_buf, 1);
			// write(1, "\n", 1);
			// close(pipefd[0]); //close read end
			// std::cout << pipe_buf << std::endl;
			// if(total_con == *phase2)
			// {
			// 	std::cout << "End of Phase 1 for the admissions office" << std::endl;
			// }
		}

		if(pid == 0) //you are child... process your data
		{
			// close(pipefd[0]); //read end close
			// close(listener);
			
			nbytes = recv(newfd, buffer, sizeof(buffer), 0);
			if(buffer[0] != 's'){ //department

				dep_info my_dep;
				my_dep.name = buffer[0];
				std::string program; //temp
				
				//generic algorithm for each child
				std::stringstream ss(buffer); 
				while(getline(ss, program, '#'))
				{
					float GPA;
					ss >> GPA;
					my_dep.programs.insert(std::pair<std::string, float>(program, GPA));
				}
		
				if(my_dep.programs.size() == 3) //assumed that each department has at least 3 programs
				{
					std::cout << "Recieve the program list from Department " << my_dep.name << std::endl;
					close(newfd); //End of Phase 1
					// *phase2 =1 ;
					// write(pipefd[1], phase2, sizeof(phase2));
					// close(pipefd[1]);
				}
			}
			else
			{
				//std::cout << "The admission office has TCP port: " << my_port << " and IP address: " << ip_addr << std::endl;
				std::stringstream ss(buffer);
				std::string application;
				stu_info stu_app;
				int i=0;
				while(std::getline(ss, application, ' '))
				{
					if(i == 0)//student name
					{
						stu_app.name = application;
					}else if(i == 1) //student's GPA
					{
						std::istringstream ss(application);
						ss >> stu_app.GPA;
					}else //interests 
					{
						stu_app.interests.push_back(application);
					}
					i++;
				}
				std::cout << "Admissions office recieved the application from " << stu_app.name << std::endl;
				//student(i, stu_app);
				//stu_map.insert(std::pair<std::string, stu_info>(stu_app.name, stu_app)); //creating the data base
			}
						
		}

	}

	//Part 2 - you are the child with the department info
	if(pid == 0)
	{

	}

	return 0;
}

/*
	if(depart_dataA.size() == 3 && depart_dataB.size() == 3 && depart_dataC.size() == 3)
	{
		std::cout << "End of Phase 1 for the admissions office" << std::endl;
	}
*/

/*
	fd_set master; //master client list
	fd_set read_fds; //temp client list for select()
	int fdmax; //max # of clients we'll accept
*/

// bool admissions::conn()
// {

// 	if(listen(listener, 10) == -1) //listen to only 1 connection
// 	{
// 		perror("listening error");
// 		exit(3);
// 	}

// 	FD_SET(listener, &master); //add listener to master set

// 	fdmax = listener; //keep track of biggest descriptor -- only 1 so far

// 	for(;;){
// 		read_fds = master; //copy
// 		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
// 			perror("select error");
// 			exit(4);
// 		}

// 	//run through connections looking for data
// 	for(int i=0; i<= fdmax; i++){
// 		if(FD_ISSET(i, &read_fds)){ //there's data
// 			if(i == listener){
// 				//handle new connections
// 				addrlen = sizeof(remoteaddr);
// 				newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
				
// 				if(newfd == -1){
// 					perror("accept");
// 				}else{
// 					FD_SET(newfd, &master); //add to master set
// 					if(newfd > fdmax){
// 						fdmax = newfd; //keeps track of the max
// 					}
// 				}
// 			}else{
// 				if((nbytes = recv(i, buffer, sizeof(buffer), 0)) <= 0){
// 					close(i); 
// 					FD_CLR(i, &master);
// 				}else{
// 						if(!phase2)
// 						{
// 							//data from department
// 							std::stringstream ss(buffer);
// 							std::string program;
// 							float GPA;
// 							if(buffer[0] == 'A'){ //department A
// 								getline(ss,program, '#');
// 								ss >> GPA;
// 								depart_dataA.insert(std::pair<std::string, float>(program, GPA));
// 								if(depart_dataA.size() == 3)
// 									std::cout << "Recieve the program list from Department A" << std::endl;
// 							}
// 							else if(buffer[0] == 'B'){ //department B
// 								getline(ss,program, '#');
// 								ss >> GPA;
// 								depart_dataB.insert(std::pair<std::string, float>(program, GPA));
// 								if(depart_dataB.size() == 3)
// 									std::cout << "Recieve the program list from Department B" << std::endl;
// 							}else{ //department C
// 								getline(ss,program, '#');
// 								ss >> GPA;
// 								depart_dataC.insert(std::pair<std::string, float>(program, GPA));
// 								if(depart_dataC.size() == 3)
// 									std::cout << "Recieve the program list from Department C" << std::endl;
// 							}
					
// 							if(depart_dataA.size() == 3 && depart_dataB.size() == 3 && depart_dataC.size() == 3)
// 							{
// 								std::cout << "End of Phase 1 for the admissions office" << std::endl;
// 								phase2 = true;
// 							}
// 						}
// 						else 
// 						{
// 							std::cout << "The admission office has TCP port: " << my_port << " and IP address: " << ip_addr << std::endl;
// 							std::stringstream ss(buffer);
// 							std::string application;
// 							stu_info stu_app;
// 							int i=0;
// 							while(std::getline(ss, application, ' '))
// 							{
// 								if(i == 0)//student name
// 								{
// 									stu_app.name = application;
// 								}else if(i == 1) //student's GPA
// 								{
// 									std::istringstream ss(application);
// 									ss >> stu_app.GPA;
// 								}else //interests 
// 								{
// 									stu_app.interests.push_back(application);
// 								}
// 								i++;
// 							}
// 							std::cout << "Admissions office recieved the application from " << stu_app.name << std::endl;
// 							//student(i, stu_app);
// 							stu_map.insert(std::pair<std::string, stu_info>(stu_app.name, stu_app)); //creating the data base
// 						}
// 				}
// 			}
// 		}
// 	}
	
// }
		
// }