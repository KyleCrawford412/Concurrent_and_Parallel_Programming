// Kyle Crawford
// Concurrent and Parallel Programming
// Spring 2023
// Project 3



#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <chrono>
#include <thread>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>

using namespace std;

// global variables
const unsigned MAXBUFFER = 1024;
int socket_fd;




// struct for messages
struct message{
    string username;
    string receiver;
    string command;
    string message;
	//int messageNumber;
};

// function declarations 
void create_message(int socket_fd, message new_message);
void tokenizer(const string &input, vector<string> &tokens, char delim);
message message_processor(const string &input);
void signal_handler(int s);


int main(int argc, char **argv){
	
	// variable declarations
	struct addrinfo address_hints, *address_save, *address_info;
	
    string user_name, user_input, fin, port, host;
	bool user_login = false;
    //int messageNumber = 0;
	int max_fd, result, flag;
    char buffer[MAXBUFFER];
    ifstream config_file;
    fd_set read_set, initial_set;
	
    
	// signal handler
    struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGINT, &sa, nullptr);

	// should have config file with command
    if(argc != 2){
        cout << argv[0] << " config_file" << endl;
        exit(1);
    }

	// open the config file to check for port and host
    config_file.open(argv[1]);

	// check for keywords servport and servhost as described by assignment
    for(int i = 0; i < 2; i++){
		
        config_file >> fin;

        if(fin == "servport:"){
            config_file >> port;
        }
        if(fin == "servhost:"){
            config_file >> host;
        }
    }
	
	// if not given format is invalid
    if(port.empty() || host.empty()){
        cout << "Error: invalid format for the config file." << endl;
        return 0;
    }
	
	// close config file
    config_file.close();

	
	// reserve a memory block for a struct
	// reserved memory needs to match size
    memset(&address_hints, 0, sizeof(struct addrinfo));
	
	
	// set family to unspecified 
    address_hints.ai_family = AF_UNSPEC;
	
	// setting the type to TCP stream sockets
    address_hints.ai_socktype = SOCK_STREAM;

	// get address info
    if((result = getaddrinfo(host.c_str(), port.c_str(), &address_hints, &address_info)) != 0){
        cout << "getaddrinfo wrong: " << gai_strerror(result) << endl;
        return 1;
    }

	
    address_save = address_info;
	flag = 0;

	// iterate through the address info and try to connect to the server
	while(address_info != nullptr){
		socket_fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
		if(socket_fd >= 0){
			if(connect(socket_fd, address_info->ai_addr, address_info->ai_addrlen) == 0){
				flag = 1;
				break;
			}
			close(socket_fd);
		}
		address_info = address_info->ai_next;
	}


	
    freeaddrinfo(address_save);

	// check if the connection was successful
    if(flag == 0){
        cerr << "Error: unable to connect." << endl;
        return 1;
    }
	
	// initialize file descriptor sets for select()
    FD_ZERO(&initial_set);
    FD_SET(STDIN_FILENO, &initial_set);
    FD_SET(socket_fd, &initial_set);
    max_fd = max(socket_fd, STDIN_FILENO) + 1;

	// main loop for handling user input and server messages
    while(!cin.eof()){
        read_set = initial_set;
        select(max_fd, &read_set, nullptr, nullptr, nullptr);
		
		// make sure the socket is ready to read
        if(FD_ISSET(socket_fd, &read_set)){
			
            message return_message;
			
			// check that read didnt fail
            if(read(socket_fd, buffer, MAXBUFFER) == 0){
                cout << "Error: the server  has crashed.\n";
                return 0;
            }
			
			// Process the received message
            return_message = message_processor(buffer);
			
			// handles messages from the server
			// dont echo users messages
            if(return_message.command == "chat" && return_message.username != user_name){
                cout << return_message.username << " >> " << return_message.message << endl;
            } 
			else if(!return_message.message.empty()){
                cout << return_message.message << endl;
            }

            //if(return_message.command != "chat" && return_message.messageNumber != messageNumber - 1){
                //cout << "Error: the messages are out of sync.\n";
			//}
        }
		// check for available input to process
        if(FD_ISSET(STDIN_FILENO, &read_set)){
            message new_message;
            vector<string> tokens;
			
			// initialize the message struct
            new_message.username = user_name;
            //new_message.messageNumber = messageNumber;
            new_message.message = "";
            new_message.receiver = "";
			
			// reads input and stores it in user_input variable
            getline(cin, user_input);
			
			// split the user input into tokens
            tokenizer(user_input, tokens, ' ');
			
			// set the command field to the first token
            new_message.command = tokens[0];

            // Using a switch statement with a hash function for string comparison
            // and creating messages to the server based on user inputs
            enum command { exit, login, logout, chat, invalid };
            auto hash_command = [](const string &str) {
                if (str == "exit") return exit;
                if (str == "login") return login;
                if (str == "logout") return logout;
                if (str == "chat") return chat;
                return invalid;
            };

			// check for the user command and check for validity for each command
            switch(hash_command(new_message.command)){
                case exit:
                    if(!user_login && tokens.size() == 1){
                        create_message(socket_fd, new_message);
                        close(socket_fd);
                        return 0;
                    } 
					// user tries to exit the program without logging out
					else if(user_login && tokens.size() == 1){
                        cout << "Logout required before exiting.\n";
                    } 
					else{
                        cout << "Number of arguments is invalid for 'exit'.\n";
                    }
                    break;
                case login:
                    if(!user_login && tokens.size() == 2){
                        user_name = tokens[1];
                        new_message.username = user_name;
                        create_message(socket_fd, new_message);
                        //messageNumber++;
                        user_login = true;
                    } 
					// user is already logged in
					else if(user_login && tokens.size() == 2){
                        cout << "You are already logged in.\n";
                    } 
					else{
                        cout << "Number of arguments is invalid for 'login'.\n";
                    }
                    break;
                case logout:
				// user tries to logout without logging in
                    if(!user_login && tokens.size() == 1){
                        cout << "You are not logged in.\n";
                    } 
					else if(user_login && tokens.size() == 1){
                        create_message(socket_fd, new_message);
                        //messageNumber++;
                        user_login = false;
                    } 
					else{
                        cout << "Number of arguments is invalid for 'logout'.\n";
                    }
                    break;
                case chat:
					// user tries to send message without logging in
    if (!user_login) {
        cout << "Login is required to send a message.\n";
    }
    // user sending message with no target should send to everyone on the server
    else if (user_login && tokens.size() >= 2) {
        if (tokens[1][0] == '@' && tokens.size() >= 3) {
            tokens[1].erase(0, 1);
            new_message.receiver = tokens[1];
            new_message.message = tokens[2];
            for (size_t i = 3; i < tokens.size(); ++i) {
                new_message.message += " " + tokens[i];
            }
            create_message(socket_fd, new_message);
            //messageNumber++;
        }
        else {
            new_message.message = tokens[1];
            for (size_t i = 2; i < tokens.size(); ++i) {
                new_message.message += " " + tokens[i];
            }
            new_message.receiver = "";
            create_message(socket_fd, new_message);
            //messageNumber++;
        }
    }
    // not enough arguments for chat
    else {
        cout << "Number of arguments is invalid for 'chat'.\n";
    }
    break;
                case invalid:
                default:
                    cout << "The command was invalid.\n";
                    break;
            }
        }
    }
    return 0;
}


// separate input into tokens
void tokenizer(const string &input, vector<string> &tokens, char delim) {
    
	// clear tokens
	tokens.clear();
	
    size_t i = 0;

    while(i < input.size()){
        string temp;

        if(input[i] == delim){
            i++;
            continue;
        } 
		else{
            if(tokens.size() < 2){
                while (i < input.size() && input[i] != delim){
                    temp.push_back(input[i]);
                    i++;
                }
            } 
			else{
                while(i < input.size()){
                    temp.push_back(input[i]);
                    i++;
                }
            }
            tokens.push_back(temp);
        }
    }
}

// geneate a struct for the message and then send it to the server
void create_message(int socket_fd, message new_message) {
    string output;
	
	cout << "" << endl;
	
	
    output = "username: ";
    output += new_message.username + "\n";
    //output += "messageNumber: ";
    //output += to_string(new_message.messageNumber) + "\n";
    output += "receiver: ";
    output += new_message.receiver + "\n";
    output += "command: ";
    output += new_message.command + "\n";
    output += "message: ";
    output += new_message.message;

    if(output.size() >= MAXBUFFER){
        cout << "The message is too big.\n";
	}
    else{
        write(socket_fd, output.c_str(), output.size() + 1);
	}
}

// Process a received message and create a message struct
message message_processor(const string &input) {
    
	// local variables
	message output;
    size_t i, j;
	
	
    i = input.find(':');
    j = input.find('\n');
    output.username = input.substr(i + 2, j - i - 2);
	
	
    //i = input.find(':', j);
    //j = input.find('\n', j + 1);
    //output.messageNumber = stoi(input.substr(i + 2, j - i - 2));
	
	
    i = input.find(':', j + 1);
    j = input.find('\n', j + 1);
    output.receiver = input.substr(i + 2, j - i - 2);
	
	
    i = input.find(':', j + 1);
    j = input.find('\n', j + 1);
    output.command = input.substr(i + 2, j - i - 2);
	
	
    i = input.find(':', j + 1);
    output.message = input.substr(i + 2, input.size() - i - 2);

    return output;
}

void signal_handler(int s) {
    if (s == SIGINT) {
        cout << "SIGINT received. Closing the connection and exiting.\n";
        if (socket_fd != -1) {
            close(socket_fd);
        }
        exit(0);
    }
}


