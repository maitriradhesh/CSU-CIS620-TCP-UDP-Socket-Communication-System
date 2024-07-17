This project is about understanding TCP/UDP sockets and also to learn C and GO languages. We have to write a client-server program in which the client can contact the db server to access the database. I also need to write a service-map program which resides at a well-known UDP socket. The socket is a 5-digit number 2XXXX where XXXX is last 4-digits of  our CSU ID (in our case, 1749). 

Let’s begin with the servicemap.go. 
The service-map server (written in GO) accepts registration from the db server (written in C) and advertises socket address to the prospective client (written in GO). When a database starts, it first requests a local TCP Port. Then, it broadcasts a registration message in UDP. 

For example: PUT BANK620 tcp-portnum

The service-mapper will create a GO-routine (i.e. a thread) to handle the message. That is, it stores the service name BANK620 along with the IP address and Port Number of the db server in its table, and replies a message "OK" to the db server. When a client starts, it broadcasts a request message. 

For example: GET BANK620

The service-mapper will create a GO-routine to return a datagram containing the information of the db server (i.e. dotted IP and Port Number in ASCII). 
Example of datagram: 137.148.204.16∶8745
 
So, to explain the code of servicemap.go,

checkError(err error) function:

Purpose: A utility function that checks if an error has occurred (err is not nil). If an error is detected, it prints the error message and exits the program.
	Parameters: err - the error to check.
	Operation: If err is not nil, it prints "Error occured:" followed by the error message, and then exits the program with an exit code of 0.

HandleMsg(ServerCLNS*net.UDPConn,adr*net.UDPAddr,buf string) function:
	Purpose: This is a Go routine (a lightweight thread) designed to handle incoming UDP messages asynchronously. It parses the message, distinguishes between PUT and GET requests, and acts accordingly.
	Parameters:
	ServerCLNS*net.UDPConn: A pointer to the UDP connection on which the server is listening. It's used to send replies back to the client.
	adr*net.UDPAddr: The address of the client that sent the message. It's used to send the reply to the correct client.
	buf string: The message received from the client, converted to a string for processing.
	Operation: The function first prints the source of the message and its content. It then splits the message into tokens using spaces as delimiters. Depending on whether the first token is "PUT" or "GET", it performs different actions:
	If the request is a "PUT", it stores the sender's IP and the specified port (from the message) in a global variable 'serv_table' with the key "server". It then sends an "OK" reply to the sender.
	If the request is a "GET", it retrieves the stored service information from 'serv_table' and sends it back to the requester.
	A global mutex 'mutex' is declared alongside 'serv_table'. This mutex is used to ensure that updates to 'serv_table' (in the "PUT" request handling) are done with mutual exclusion, preventing race conditions.
	In the HandleMsg function, before the 'serv_table'map is updated, 'mutex.Lock()' is called to acquire the lock. After the update is completed, 'mutex.Lock()' is called to release the lock, allowing other goroutines to acquire the lock and safely update the map.

main() function:

Purpose: The main entry point of the program. It sets up the UDP server and enters a loop to continuously read messages from clients.
	Operation:
	It first resolves the UDP address for the server to listen on port 21749 on all interfaces (":21749").
	Then, it creates a UDP socket for the server with net.ListenUDP and starts listening for incoming UDP packets on the resolved address.
	A byte slice 'buf' is created to read incoming packets.
	The server enters an infinite loop where it reads from the UDP connection into 'buf'. For each read operation, it launches a new Go routine 'HandleMsg' to handle the message asynchronously. This allows the server to handle multiple requests in parallel without blocking on a single request.
	If an error occurs during the read operation, it prints the error.
	Global Variables:
	'serv_table:make(map[string]string)' : A global variable that acts as a simple in-memory key-value store. In this context, it's used to store the IP and port information of a service, with the key presumably being the service name (although the code currently hardcodes the key as  "server").
	'fmt^': The 'fmt^' package is used for formatting I/O operations, like reading input and printing output to the terminal. In this program, it's used to print error messages and logs to the console with functions like 'fm〖t.Println〗^'.
	'net^': The 'net^' package provides a portable interface for network I/O, including TCP/IP, UDP, domain name resolution, and Unix domain sockets. In this UDP server example, it's used extensively for all network-related operations:
	'ne〖t.ListenUDP〗^' creates a UDP listener for the server, allowing it to receive UDP packets sent to the specified address.
	'ne〖t.UDPConn〗^'is a type representing a UDP network connection. This program uses it to read from and write to UDP connections.
	'ne〖t.UDPAddr〗^' represents the address of a UDP end point.


	Imports:
	'os': The 'os' package provides a platform-independent interface to operating system functionality. It's used for calling 'os.Exit(0)' to exit the program with a status code of 0 when an error occurs.
	'strings^': The'strings^' package implements simple functions to manipulate UTF-8 encoded strings. It's used in this program for 'string〖s.Split〗^' to split the received UDP message into tokens based on spaces. This parsing helps in identifying and processing the command ("PUT" or "GET") and its arguments from the message.
	'sync': The 'sync' package provides basic synchronization primitives such as mutual exclusion locks ('sync.Mutex'). The program utilizes 'sync.Mutex' to ensure that updates to the global 'serv_table' variable are done safely across multiple goroutines, preventing race conditions or data corruption.

In the  program, 'string〖s.Split〗^' is used to split the UDP message received from clients based on spaces (" "). This is particularly useful for handling simple text-based commands where the command and its arguments are separated by spaces.

Hence, to summarize, the program demonstrates a basic UDP server that can register and retrieve service location data (IP and port) based on simple text commands. It leverages Go's concurrency model by using Go routines to handle incoming requests in parallel, allowing for efficient processing of UDP packets without blocking on I/O operations. This makes it well-suited for scenarios where quick responses to lightweight requests are required, such as service discovery or registration mechanisms in a distributed system.
 
We had the copy the database db24 from the directory. Each record in the database has the following fixed format:

struct record { 
	int acctnum; 
	char name[20]; 
	char phone[16];
	int age; 
}accinfo; 

The record is stored in binary (not ASCII format). Hence, to see the file content, we need to use the od command.

Let’s continue with the client.go. 
The client gets a command from the standard input and then forwards the command with two newline characters appended at the end to the server. 
For example: qry acctnum\n\n
	          upd acctnum amount\n\n
	          quit\n\n
The qry command requests the server to get and reply the record for a person. The upd command asks the server to add an amount to a person’s record in the database.
 
So, to explain the code of client.go,
The Go program is designed to discover a service via UDP broadcast and then establish a TCP connection to interact with it. Here's a function-by-function explanation:

Network Communication and Discovery

UDP Broadcast for Service Discovery
	Purpose: The initial part of the program deals with discovering a service on the network. It sends a UDP broadcast message to a predefined broadcast address and port. Broadcasting is used here because the client does not know the exact address of the server it needs to communicate with. The message "GET BANK620 " is intended for services listening on the network that recognize this request format and are configured to respond to it.
	Implementation:
	'ne〖t.ResolveUDPAddr〗^' is used twice to resolve both the local address for binding the UDP socket ('〖0.0.0.0:0〗^') and the broadcast address ('〖137.148.205.255:21749〗^'). The former allows the operating system to automatically choose an available port and interface, while the latter specifies the target address for the broadcast message.
	'ne〖t.ListenUDP〗^' creates a UDP socket bound to the resolved local address. This socket is used for sending the broadcast message and receiving the reply.
	'conn〖.WriteToUDP〗^' sends the broadcast message. The use of 〖'WriteToUDP〗^' allows specifying the destination address for each message sent, which is necessary for broadcasting.
	The call to 'conn〖.SetReadDeadline〗^' prevents the program from waiting indefinitely for a response. This is particularly important in a broadcast scenario where a response may not be guaranteed.

TCP Connection for Reliable Communication
	Purpose: After receiving the service's address through UDP, the program establishes a TCP connection to the service. TCP is chosen for the subsequent communication because it provides a reliable, ordered, and error-checked delivery of a stream of bytes between the program and the service.
	Implementation:
	The service's IP address and port number are parsed from the UDP reply and used to form an address string in the "<ip>:<port>" format.
	'ne〖t.Dial〗^' establishes a TCP connection to the service using this address. The established connection ('〖tcp_conn〗^') is then used for two-way communication.

User Interaction and Protocol Handling
	Purpose: The program enters an interactive loop where it reads commands from the user, sends them to the server, and displays the server's responses. This part simulates a basic command-line client for the service.
	Implementation:
	A 'bufio〖.NewReader〗^' is created to read from 'os〖.Stdio〗^', allowing the program to handle user input line-by-line.
	Inside the loop, each command line read from the user is appended with two newline characters before being sent. This is crucial if the server expects each command to be terminated by two newlines, which might be part of the service's protocol for delineating commands.
	The program listens for a response after each command is sent. It uses a byte slice 'res' to read the response from the server. The response is then converted to a string and printed to the console, providing immediate feedback to the user.

Detailed Points:
	Error Handling and Exit Strategy: The program makes liberal use of 'fm〖t.Println〗^'for printing errors before exiting. This approach ensures that any issues encountered during execution are immediately surfaced to the user. However, the program exits with a status code of 0 on errors, which is unconventional. Typically, a non-zero exit status indicates an error. Modifying 'os.Exit(0)' to 'os.Exit(1)' or another non-zero value for errors would be more aligned with common practices.
	Network Timeouts and Delays: Setting a read deadline on the UDP socket is a critical aspect of handling network timeouts effectively. It ensures that the program remains responsive even if no service is available to respond to the broadcast message. This pattern is essential in network programming, especially in discovery mechanisms where the presence of a responding service cannot be guaranteed.
	Converting Reply to IP and Port: The parsing logic assumes the reply is in a very specific format ('〖IP:port〗^'). While this works under controlled conditions, real-world scenarios might require more robust parsing with error checking to handle unexpected or malformed responses gracefully.
	Sending Commands with Newlines: Appending two newline characters to each command is a simple yet effective way to meet protocol requirements with minimal changes to the codebase. It demonstrates how client-side adjustments can be made to accommodate server expectations.

Imports:
	'〖bufio〗^': The '〖bufio〗^' package provides buffered I/O. It wraps around an 'io.Reader' or 'io.Writer' interface to create buffered readers ('〖bufio.Reader〗^') and writers (bufio.Writer'), facilitating efficient reading and writing by minimizing disk or network I/O operations. In this context, it's likely used to read input from the user or possibly from a file or network connection in a buffered manner.
	'〖fmt〗^': The '〖fmt〗^' package implements formatted I/O with functions analogous to C's 'printf' and 'scanf'. It's used for formatting strings, reading input, and printing output to the console. It's one of the most frequently used packages for handling text output for logs, debugging, or user interaction.
	'〖net〗^': The '〖net〗^' package provides a portable interface for network I/O, including TCP/IP, UDP, domain name resolution, and Unix domain sockets. This program uses it for network communication tasks like listening for connections, connecting to servers, and transmitting data over the network.
	'〖os〗^': The '〖os〗^' package provides a platform-independent interface to operating system functionality, including file and process management. It's often used to access command-line arguments, handle environment variables, and work with the filesystem. In networked applications, it might be used for reading environment variables or handling files (for configuration or logging).
	'〖strconv〗^': The '〖strconv〗^' package implements conversions to and from string representations of basic data types. It is commonly used to convert strings to numerical types (like integers or floats) and vice versa, which is especially useful for parsing numeric data received as text from user input or over the network.
	'〖strings〗^': The '〖strings〗^' package provides functions for manipulating UTF-8 encoded strings. It includes functionality for string searching, trimming, replacing, splitting, and joining, among others. This can be particularly useful for parsing and constructing text protocols, handling user input, or generating text output.
	'〖time〗^': The 'time' package supplies functionality for measuring and displaying time. It includes functions to retrieve the current time, manipulate time and date values, format and parse time representations, and handle durations and time zones. In network applications, it might be used for setting timeouts, measuring elapsed time, or scheduling future actions.

These imported packages collectively enable the program to perform a wide range of tasks: managing network connections, processing text, interacting with the operating system, and handling timing and scheduling tasks.

This Go program exemplifies how to combine UDP and TCP protocols for service discovery and communication, handling user input in real-time, and ensuring robust error management in network applications.










Code of client.go:
 
 


Next, the server.c. 
The server returns the result via an ASCII message with the characters \n\n appended at the end and the TCP connection should be kept alive until getting the quit command.
When the server establishes a connection with a client, it forks a child process to handle the requests. The child process has to use the operations lseek and lockf in UNIX. The lseek function can locate the position to be rewritten in a file, while the lockf can lock a file (or even a record) to prevent multiple clients from updating the same record simultaneously. The parent process should use a signal-changing routine to clean up any child process which has terminated.

So, to explain the code of server.c,
This C program serves as a server application that manages a simple database (a file named "db24") of accounts. It allows clients to update and query account records through TCP connections, while also broadcasting its presence via UDP. The server uses a multi-process approach to handle client requests concurrently. Here's a breakdown of its key functions:

signal_catcher(int the_sig) function:
	Purpose: Catches 'SIGCHLD' signals to clean up zombie processes left by terminated child processes. This is essential in a server that forks a new process for each client connection.
	Parameters: the_sig - the signal number (not used in the function but necessary for the signature).
	Operation: Calls 'wait(0)' to clean up zombie processes.
	More Information:
	Zombie Processes: When a child process terminates, it still remains in the system with an exit status for the parent to read. Until the parent reads this status (using 'wait'  or similar functions), the child remains in a "zombie"  state, consuming resources.
	Preventing Resource Leaks: By catching 'SIGCHLD' and calling 'wait(0)' the server ensures that terminated child processes do not linger and consume system resources.

query_database(int acctnum,char*buffer) function:
	Purpose: Queries the "db24" database file for a record matching ‘acctnum' and formats a reply message with the account details.
	Parameters:
	'acctnum' - the account number to query.
	'buffer' - a string buffer to store the reply message.
	Operation: Opens the database file, reads records sequentially, and checks for a matching account number. If found, it formats a message with the account details into 'buffer'.
	More Information:
	Database Access: The database is a simple file ('db24^'), where each record is a 'struct record'. instance. The function sequentially reads these records using 'read()'.
	Record Matching: If a record matching acctnum'  is found, it formats a response string including the account's name, account number, and value into 'buffer'. If no matching record is found, an error message is formatted into 'buffer' instead.
	Resource Management: The database file is opened at the start and closed before returning, ensuring that file descriptors are not leaked.
	
update_database(int acctnum,double value) function:
	Purpose: Updates the value associated with 'acctnum'  in the database by adding 'value'  to it.
	Parameters:
	'acctnum' - the account number to update.
	'value' - the amount to add to the account's value.
	Operation: Similar to query_database, but it locks the record for writing, updates the account's value, and writes the updated record back to the file.
	More Information:
	Locking for Write Access: It uses 'lockf(fd,F_LOCK,sizeof(struct record))' to lock the record for exclusive access. This is crucial for preventing race conditions where two or more server processes might attempt to update the record simultaneously.
	Record Updating: Upon finding the matching record, the function updates the account's value and writes the modified record back to the database file using 'write()'.
	Unlocking: After updating, the record is unlocked with 'lockf(fd,F_ULOCK,sizeof(struct record))', allowing other processes to access it.

get_ip_address(char*buffer,size_t buflen) function:
	Purpose: Retrieves the server's first non-loopback IPv4 address and stores it in 'buffer'.
	Parameters:
	'buffer' - a buffer to store the IP address.
	'beflen' - the size of 'buffer'.
	Operation: Iterates over the server's network interfaces to find the first non-loopback IPv4 address. Uses 'getifaddrs' to get a list of interfaces and 'getnameinfo' to convert the first suitable address to a string.
	More Information:
	Interface Iteration: It iterates over all network interfaces obtained via 'getifaddrs(&ifaddr)'. For each interface, it checks whether it is not a loopback ('!(ifa->ifa_flags & IFF_LOOPBACK)')  and is of family 'AF_INET'. 
	Address Retrieval: For the first suitable interface, it uses 'getnameinfo()' to convert the interface's address to a string format, storing it in 'buffer'.

main() function:
The main function in this C program orchestrates the setup and operation of a server that communicates with clients using TCP for request handling and UDP for service announcement. It demonstrates a combination of network programming techniques, process management, and simple file-based database operations. Here's a detailed breakdown of its operation:

Signal Handling for Zombie Process Cleanup
	The program registers a signal handler for 'SIGCHLD' using 'signal(SIGCHLD,signal_catcher)'. 'SIGCHLD' is sent to a parent process whenever one of its child processes terminates. The signal handler, 'signal_catcher', uses 'wait(0)' to wait for all child processes that have terminated, thus cleaning up ("reaping") any zombie processes. Zombie processes are child processes that have terminated but still occupy an entry in the process table.

TCP Socket Creation and Binding
	A TCP socket is created with 'socket(AF_INET,SOCK_STREAM,0)'. This socket listens for incoming client connections.
	The server's address structure '(serv_adr)' is configured to listen on any interface ('INADDR_ANY') and to let the system dynamically assign a port ('serv_adr.sin_port =0').
	The socket is bound to this address with 'bind()'. Binding assigns the specified address to the socket, making it a named socket that can receive connections.
	The program uses 'getsockname' to retrieve the assigned port number after binding, since it was set to '0' to allow dynamic assignment. This port number is later broadcasted over UDP for service discovery.

UDP Broadcasting for Service Announcement
	The server announces its presence on the network by sending a broadcast message via UDP. This message contains the service name "BANK620" and the dynamically assigned TCP port. Clients can listen for this broadcast to discover the server's IP address and port.
	A UDP socket is configured with 'setsockopt(sk,SOL_SOCKET,SO_BROADCAST,…)' to enable broadcasting.
	The broadcast message is sent using 'sendto()', targeting a predetermined broadcast address and UDP port.

Listening for TCP Connections and Forking Child Process
	The server listens for incoming TCP connections with 'listen(orig_sock,5)', specifying a backlog of 5, which is the number of pending connections that can be queued.
	For each incoming connection, 'accept()' creates a new socket ('new_sock') for communication with the connecting client. The server then forks a new process to handle this client.
	The child process handles client requests in a loop, reading messages with  'recv()', processing them, and sending responses with 'send()'. It supports two types of messages: queries ('qry') for account information and updates ('upd') to account values.
	The parent process continues to accept new connections, and the cycle repeats.

Error Handling and Resource Management
	Throughout its operation, the program checks for errors (e.g., failure to create a socket, bind, listen, accept connections, read from or write to the database). On encountering an error, it prints an error message and exits or continues as appropriate.
	Child processes are responsible for closing their dedicated client sockets ('new_sock')  before terminating to ensure resources are released properly.



Imports:
	'<sys/types.h>': Provides definitions of data types like 'pid_t' (used for process IDs) and 'size_t' (used for sizes of objects). It's often required when using other system call-related header files.
	'<sys/socket.h>': Defines functions and data structures needed for socket programming, enabling the program to perform network communication over TCP or UDP.
	'<netinet/in.h>': Contains constants and structures needed for internet domain addresses. For instance, it defines 'struct sockaddr_in'which is used for IPv4 addresses.
	'<arpa/inet.h>': Provides declarations for functions related to Internet operations, including functions for converting between host and network byte order, and for manipulating numerical IP addresses ('inet_addr,'inet_ntoa').
	'<net/if.h>': Used for the configuration and management of network interfaces; provides data structures and constants used with network interface control operations. For example, it defines 'IFF_LOOPBACK' used to check if an interface is a loopback interface.
	'<sys/un.h>': Defines structures and constants needed for UNIX domain sockets, which are sockets used for inter-process communication (IPC) on the same host.
	'<netdb.h>': Defines functions and structures for network database operations, such as translating a hostname to an IP address, which is part of the Domain Name System (DNS) resolution process.
	'<unistd.h>': Provides access to POSIX operating system API, including numerous essential functions like 'read', 'write', 'close', and 'fork'.
	'<stdlib.h>': Standard General Utilities Library. It includes functions involving memory allocation, process control, conversions, and others. Functions like 'exit' and 'atoi' are defined here.
	'<stdio.h>': Standard Input and Output Library. It provides functionalities for file and data stream input and output operations. This is where functions like 'printf'  and 'sprintf' come from.
	'<ifaddrs.h>': Defines the 'getifaddrs'  function and related structures. This function retrieves the network interfaces of the local system, which can be used to find IP addresses assigned to the server.
	'<signal.h>': Provides facilities for setting handlers for different types of signals. Signals are used for inter-process communication, with 'signal' and 'sigaction' being common functions for setting signal handling functions.
	'<wait.h>': Includes declarations for waiting for process termination. It defines the 'wait' and 'waitpid' functions which are used for synchronizing with terminated child processes, thus preventing zombies.
	'<ctype.h>': Contains functions to test and map characters. This includes character classification functions like 'isdigit'  or 'isalpha', and functions to convert between uppercase and lowercase.
	'<string.h>': Contains functions for handling strings and arrays. It's used for operations like comparing, copying, and concatenating strings, with functions like 'strlen'  , 'strcmp', 'strcpy'.
	'<fcntl.h>': Defines operations on file descriptors like opening, locking files. It provides the 'open', 'fcntl', and 'lockf'functions, allowing control over file descriptors beyond simple reading and writing.


Lastly, makefile.
All that we had to do here was to put together the compiling and executing commands. Along with that, we also need the all and clean commands.


Output:

In order to run the program, we need 3 different OS (machines). The machines that I choose were:
	spirit to run the servicemap.
	beethoven to run the server.
	brahms to run the client.

To do that, I used remote login. I logged into my spirit account and I used ssh command in order to switch to other machines, beethoven and brahms.

While we begin to run the program, there is a particular order that should be followed so that the execution goes correct. If the order is not followed, then the code will not run. Initially, while running, we first ran the server.c program. As the servicemap.go file had not yet been executed, the message was not getting broadcasted as required and hence, the client.go program would throw an error. 

Hence, to make the order:
	Firstly, we need to make the files. This will create the necessary executable files which are required to run the files.
	Next,  run the servicemap.
We need to run the servicemap.go file first as this is the file which stores the IP : Port data in its dictionary. As the data needs to be stored, the file that stores the data should be run first.

Next, we need to run the server.

Once we run the server, it will try to contact the servicemap to register and will receive a registration confirmation. 
The message printed on the terminal of the server is: Registration OK from IP address 
(IP address will be that of the servicemap)
In this case, the IP address is: 137.148.205.14
	Similarly, the servicemap will also receive the broadcasted message form the server. 
The message printed on the terminal of the servicemap is: Received from IP address∶PUT BANK620 Port Number 
(IP address and the Port Number(TCP Port) will be that of the server).
The Port Number is generated dynamically.
In this case
	The IP address is: 137.148.205.18
	The Port Number is: 36523 
	After running the servicemap.go, we run the server.c file because the server is the one who broadcasts the message to PUT the data into the dictionary in the servicemap. The data here is the Port Number of TCP Port which is on the server side. This data gets stored into the dictionary created on the servicemap side along with the Ip address of the server. 

 Next, we run the client.

Once we runt the client, it will try to contact the servicemap to register and will receive the details of the service provider. 
The message printed on the terminal of the client is: Service provided by IP address at port Port Number 
(IP address and the port number (TCP Port) will be that of the server).
The Port Number is generated dynamically.
In this case
	The IP address is: 137.148.205.18
	The Port Number is: 36523 
	Similarly, the servicemap will also receive the broadcasted message form the server. 
The message printed on the terminal of the servicemap is: Received from IP address∶GET BANK620 
(IP address and the will be that of the client).
In this case, the IP address is: 137.148.205.17
	After running the servicemap.go, and the server.c, we next run the client.go file because once the server has already made the broadcast to PUT the data into the dictionary in the servicemap, the client can now retrieve the data from the servicemap file using GET. This will fetch the IP address and the port number (TCP Port) of the server from which the further database queries can be performed.

Next, we run the test cases of the queries.

The 1st query is just a normal query where we give: qry acctnum
For example: qry 11111
This is given on the client terminal. 
	On this query, a service is requested to the server and the server has a message printed. 
The message being printed is:
Service Requested from IP address 
(IP address and the will be that of the client).
In this case, the IP address is: 137.148.205.17
	This in return prints the Account holder’s Name, Account Number, Value(Balance) in their account, Phone Number and the Age of the account holder.
Hence, the output for the above query will be:
MULAN HUA 11111 99.989998 (440)687-2001 25 
All of this is on the client terminal.
After the message is printed, we have 2 newlines as told in the handout.

The 2nd query is an update query where we give: upd acctnum upd_val
For example: upd 34567 3.4
This is given on the client terminal. 
	On this query, a service is requested to the server and the server has a message printed. 
The message being printed is:
Service Requested from IP address 
(IP address and the will be that of the client).
In this case, the IP address is: 137.148.205.17
	This in return prints the Account holder’s Name, Account Number, and Value(Balance) in their account.
(It is the updated Value)
Hence, the output for the above query will be:
BILL SUN 34567 84.000000 
All of this is on the client terminal.
After the message is printed, we have 2 newlines as told in the handout.

The above updation happens with the use of mutex locks to perform atomic transaction. This method locks an account when a user is trying to update is so that no one else can make any updations. After the updation is complete, the account is unlocked. This helps us from race conditions.
	In this case, we are adding money into the account.

The 3rd query is an update query where we give: upd acctnum upd_val
For example: upd 34567-7.8
This is given on the client terminal. 
	On this query, a service is requested to the server and the server has a message printed. 
The message being printed is:
Service Requested from IP address 
(IP address and the will be that of the client).
In this case, the IP address is: 137.148.205.17
	This in return prints the Account holder’s Name, Account Number, Value(Balance) in their account, Phone Number and the Age of the account holder.
Hence, the output for the above query will be:
BILL SUN 34567 76.199997 
All of this is on the client terminal.
After the message is printed, we have 2 newlines as told in the handout.

 The above updation happens with the use of mutex locks to perform atomic transaction. This method locks an account when a user is trying to update is so that no one else can make any updations. After the updation is complete, the account is unlocked. This helps us from race conditions.
	In this case, we are removing money from the account.

The 4th query is a query where we give: quit
For example: quit
This is given on the client terminal. 
	On this query, a service is requested to the server and the server has a message printed. 
The message being printed is:
Service Requested from IP address 
(IP address and the will be that of the client).
In this case, the IP address is: 137.148.205.17
	The output for the above query will be:

It is a blank line with just a cursor at the extreme left of that line without the ability to give any more inputs. All of this is on the client terminal.

Even if we try to give further command line inputs, the query will not run. We were able to give the command line input but the service was not requested to the server.
	So far, every time we used to give an input on the command line, a service would be requested to the server but once we have quit, no further requests will be made for any service to the server.
	This indirectly tells us that the TCP connection is terminated and the client cannot request for any more service requests from the server. 

Once we terminate the server, the client reaches EOF automatically.

Status of the project: Working


Debugging:

There were multiple instances where we had located bugs. The primary ones which were found and fixed were the ones mentioned below:

 In client.go, during the address resolution to find the UDP connection’s local address, we use the 0.0.0.0 pattern. Initially, this was not being done, which lead to an erroneous calculation of the address to be used.

Initially, we used the gethostname() function to identify the IP address of the machine running, but later, we modified the code to find the IP address using an alternative approach.

We also faced an issue in the database update, where we were to query the database and then modify the contents and the display the same, we noticed that the output of both the cases were different and cannot use the same output blocks to display the values after modification.

We also faced an issue in identifying the working of the lseek() function, it did not update the data in the required manner, but later, it did perform the update operation once we understood how to move the pointer across the records in the database.

While running the setup, ideally, the servicemap was to be up and running on a machine on the LAN first and then the server had to be run for the registration to be successful. Initially, we ran the server first and then the servicemap which led to no output on the said screens and the client threw an error as well. We rectified our mistake and managed to get the correct output.

