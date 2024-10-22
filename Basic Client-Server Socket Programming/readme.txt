i. Info:
	Name: An Pham
	ID: 9477698118

ii. Compilation Steps:
	 - Log into viterbi-scf2.usc.edu
	 - g++ -o server Server.cpp -lnsl -lresolv 
	 - g++ -o client Client.cpp -lnsl -lresolv

iii. Additional Notes:
	- Run Server before Client:
		- ./server
		- ./client

iv. References:
	- Client/Server class set up:
		"Socket Programming Concepts" Slide 36
	Converting string to char array
	 	https://www.geeksforgeeks.org/convert-string-char-array-cpp/
	- How to use Socket API:
	 	http://www.beej.us/guide/bgnet/
	- Socket Programming Example:
		https://www.geeksforgeeks.org/socket-programming-cc/