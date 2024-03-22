// Kyle Crawford
// Concurrent and Parallel Programming
// Spring 2023
// Project 3


#include <iostream>
#include <list>
#include <limits.h>
#include <fstream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <signal.h>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <iterator>
#include <algorithm>

using namespace std;

// struct for the client
struct Client{
    string username;
    int socket_fd;
	bool is_logged_in;
};

// struct for messages
struct Message{
    string username;
    string receiver;
    string command;
    string message;
    //int messageNumber;
};

// function declaration
void signal_handler(int s);
Message message_processor(const string& input);
void create_message(int socket_fd, const Message& new_message);
void client_connection_handler(Client new_client);
vector<string> tokenize(const string& input);

// global variables
int server_socket_fd;
mutex client_list_mutex;
const unsigned MAXBUFFER = 1024;
shared_ptr <list<Client>> client_list;


// main function
int main(int argc, char **argv){
	
	// variable declarations
    string fin;
	Client new_client;
    ifstream config_file;
	unsigned short port;
    sockaddr_in server_address, client_address;
    thread tid;

    // list of clients to store the user data
    client_list = make_shared<list<Client>>();

    // signal handler
    if(signal(SIGINT, signal_handler) == SIG_ERR){
        cout << "Error handling signal.\n";
	}
	
    // check validity with config file
    if(argc != 2){
        cout << argv[0] << " config_file" << endl;
        exit(1);
    }
	
	// open file
    config_file.open(argv[1]);
    config_file >> fin;

	// check for port
    if(fin == "port:"){
        config_file >> fin;
        port = static_cast<unsigned short>(stoi(fin));
    } 
	// invalid format
	else{
        cout << "The config file format is invalid.\n" << endl;
        return 0;
    }
	//close file
    config_file.close();

    // create the socket
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_socket_fd == -1){
        cout << "Error making socket\n";
        exit(2);
    }

    // socket info and memory
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);
	
	
	// allow multiple connections to the address
	int optval = 1;
	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // bind socket
    if(bind(server_socket_fd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) != 0){
        cout << "Unable to bind the socket.\n";
        exit(2);
    }

    // get info for the socket
    char domainname[_POSIX_HOST_NAME_MAX + 1];
    gethostname(domainname, _POSIX_HOST_NAME_MAX + 1);
    sockaddr_in socket;
    socklen_t len = sizeof(socket);
    getsockname(server_socket_fd, reinterpret_cast<sockaddr*>(&socket), &len);

    cout << "Domain: " << domainname << endl;
    cout << "Port: " << ntohs(socket.sin_port) << endl;

    // socket listens for connections
    listen(server_socket_fd, 10);

	// listen for new connections while running
    while(true){
        socklen_t socket_len = sizeof(client_address);
        int client_socket_fd = accept(server_socket_fd, reinterpret_cast<sockaddr*>(&client_address), &socket_len);

        Client new_client{ "", client_socket_fd, false };
        thread client_thread(client_connection_handler, new_client);
        client_thread.detach();
    }

    return 0;
}


// Function that handles client connection threads
void client_connection_handler(Client new_client){
    string input;
    cout << "starting client_connection_handler.\n";
    ssize_t n;
    char buffer[MAXBUFFER];

    // add client sockets
    {
        lock_guard<mutex> lock(client_list_mutex);
        client_list->push_back(new_client);
    }

    // process messages sent by clients
    while((n = read(new_client.socket_fd, buffer, MAXBUFFER)) > 0){
        Message new_message;
        buffer[n] = '\0';

        // get struct for the message
        input = buffer;
        new_message = message_processor(input);

        // exit command
        if(new_message.command == "exit"){
            lock_guard<mutex> lock(client_list_mutex);
			// remove the user from socket and close it
            for(auto itr = client_list->begin(); itr != client_list->end(); ++itr){
                if(itr->socket_fd == new_client.socket_fd){
                    close(new_client.socket_fd);
                    cout << "The client has been closed.\n";
                    client_list->erase(itr);
                    return;
                }
            }
        }
        // login command
        else if(new_message.command == "login"){
             {
                lock_guard<mutex> lock(client_list_mutex);
                for(auto itr = client_list->begin(); itr != client_list->end(); ++itr){
                    if(itr->socket_fd == new_client.socket_fd){
                        new_client.username = new_message.username;
                        itr->username = new_message.username;
						itr->is_logged_in = true; 
                        cout << new_client.username << " has entered the chat.\n";
						new_message.command = "login";
                        new_message.message = "Server: login successful.";
                        break;
                    }
                }
            }
            create_message(new_client.socket_fd, new_message); 
        }
        // logout command
        else if(new_message.command == "logout"){
            lock_guard<mutex> lock(client_list_mutex);
			// remove the user from the socket
            for(auto itr = client_list->begin(); itr != client_list->end(); ++itr){
                if(itr->username == new_message.username){
                    cout << new_message.username << " has left the chat.\n";
                    itr->username = "";
					itr->is_logged_in = false; 
                    new_message.message = "Server: logout successful";
                    break;
                }
            }
        }
        
        else if(new_message.command == "chat"){

            bool userFound = false;

            {
				lock_guard<mutex> lock(client_list_mutex);
				for (auto itr = client_list->begin(); itr != client_list->end(); ++itr) {
					// Check if the current user is not the sender
					if (new_message.username != itr->username && !itr->username.empty()) {
						// Send to everyone in server if no target
						if (new_message.receiver == "") {
							cout << new_message.username << " sent message to " << itr->username << endl;
							create_message(itr->socket_fd, new_message);
							userFound = true;
						}
						// Send to target user
						else if (itr->username == new_message.receiver) {
							cout << new_message.username << " sent message to " << new_message.receiver << endl;
							create_message(itr->socket_fd, new_message);
							userFound = true;
							break;
						}
					}
				}
				
				if(new_message.receiver == "command:"){
					for(auto i = client_list->begin(); i != client_list->end(); ++i) {
						if(new_message.username != i->username){
							cout << new_message.username << " sent message to " << i->username << endl;
							create_message(i->socket_fd, new_message);
							userFound = true;
						}
					}
				}
				
			}	
			
			

            // if target user doesnt exist or no one else is logged in
            new_message.message = "";
            if(!userFound && !new_message.receiver.empty()){
                new_message.message = "Error: unable to find user. " + new_message.receiver;
            } 
			else if(!userFound && new_message.receiver.empty()){
                new_message.message = "Error: there are no other users online.";
            }

            create_message(new_client.socket_fd, new_message);
			
		}
	}

	// server errors
	if(n == 0){
		cout << "The client has been closed." << endl;
	} 
	else{
		cout << "There was an error." << endl;
	}

	close(new_client.socket_fd);
	
}

// custom signal handler to exit properly
void signal_handler(int s){
    if(s == SIGINT){
        close(server_socket_fd);
        cout << "The server socket has been closed.\n";
        {
            lock_guard<mutex> lock(client_list_mutex);
            for(auto itr = client_list->begin(); itr != client_list->end(); ++itr){
                close(itr->socket_fd);
                cout << "The client socket closed.\n";
            }
        }
        exit(1);
    }
}

bool is_digits(const std::string &str) {
    return std::all_of(str.begin(), str.end(), ::isdigit);
}

// process message and create a message struct
Message message_processor(const string& input){
    Message output;
    istringstream ss(input);
    vector<string> tokens{istream_iterator<string>{ss}, istream_iterator<string>{}};

    for(size_t i = 0; i < tokens.size(); ++i){
        if(tokens[i] == "username:" && i+1 < tokens.size()){
            output.username = tokens[i+1];
        } 
		//else if(tokens[i] == "messageNumber:" && i + 1 < tokens.size()){
			//try{
				//output.messageNumber = stoi(tokens[i + 1]);
			//}catch(const std::invalid_argument& e){
				//cerr << "Error: Invalid message number in the input message." << endl;
				// set the messageNumber to a default value or handle the error appropriately
				//output.messageNumber = -1;
			//}
		//} 	
		else if(tokens[i] == "receiver:" && i+1 < tokens.size()){
            if(tokens[i] == "command:"){
				output.receiver = "";
				i--;
			}
			else{
				output.receiver = tokens[i+1];
			}
        } 
		else if(tokens[i] == "command:" && i+1 < tokens.size()){
            output.command = tokens[i+1];
        } 
		else if(tokens[i] == "message:" && i+1 < tokens.size()){
            output.message = tokens[i+1];
            for(size_t j = i+2; j < tokens.size(); ++j){
                output.message += " " + tokens[j];
            }
        }
    }

    return output;
}





// generate a struct for the message
void create_message(int socket_fd, const Message& new_message){
    string output;
    output = "username: " + new_message.username + "\n";
    output += "receiver: " + new_message.receiver + "\n";
    output += "command: " + new_message.command + "\n";
    output += "message: " + new_message.message + "\n";
    //output += "messageNumber: " + to_string(new_message.messageNumber) + "\n";

    if(output.size() >= MAXBUFFER){
        cout << "The message is too big.\n";
    } 
	else{
        write(socket_fd, output.c_str(), output.size() + 1);
    }
}

vector<string> tokenize(const string& input){
    istringstream ss(input);
    vector<string> tokens{istream_iterator<string>{ss}, istream_iterator<string>{}};
    return tokens;
}


