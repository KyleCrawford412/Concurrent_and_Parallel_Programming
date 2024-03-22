// Kyle Crawford
// Concurrent and Parallel Programming
// Spring 2023
// Project 2
// Create a simple unix systems toolkit


#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <ctime>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/times.h>
#include <fcntl.h> 
#include <sys/wait.h>
#include <math.h>
#include <signal.h>
#include <errno.h>




using namespace std;

void getTokens(string input, vector<string> &tokens, char breakChar);
bool processCommands(vector<string> &tokens);
void processPiping(vector<string> &tokens);
void IORedirect(vector<string> &tokens);
void run_external_command(const string& command, const vector<string>& args);
void mycd(vector<string> tokens);
void mypwd();
void printTree(vector<string> tokens);
void navigate(string start, string path);
string find_command(string command);
void myTime(vector<string> &tokens);
void mymTime(vector<string> tokens);
void myTimeout(vector<string>& command);
void countTimes(string location, time_t now, vector<int> &hours);
void execute_command(const vector<string>& command);

// executes commands 
void execute_command(const vector<string>& command){
    vector<char*> args;
    for (const auto& arg : command) {
        args.push_back(const_cast<char*>(arg.c_str()));
    }
    args.push_back(nullptr);

    execvp(args[0], args.data());
}

// separate string from user input into tokens
void getTokens(string input, vector<string> &tokens, char delim){
	
	// reset the token vector
	tokens.clear();
	int i = 0;
	
	// process all tokens in the string
	while(1){
		string temp = "";
		
		if(input[i] == '\0'){
			i++;
			break;
		}
		else if(input[i] == delim){
			i++;
			continue;
		}
		// if input isnt a break character add it to token
		else{
			// loop through word when one starts
			while(input[i] != '\0' && input[i] != delim){
				temp.push_back(input[i]);
				i++;
			}
			tokens.push_back(temp);
		}
	}
}

// process commands 
bool processCommands(vector<string> &tokens){
	
	// check for piping and IO redirects
	bool piping = false;
	bool IO = false;
	
	
	// check for piping and IO redirects in the command
	for(int i = 0; i < tokens.size(); i++){
		if(tokens[i] == "|"){
			piping = true;
		}
		if(tokens[i] == "<" || tokens[i] == ">"){
			IO = true;
		}
	}
	
	// if piping go to piping function 
	if(piping){
		processPiping(tokens);
	}
	// check for IO redirect go to redirect function
	else if(IO){
		IORedirect(tokens);
	}
	// change directory function call
	else if(tokens[0] == "mycd"){
		mycd(tokens);
	}
	// print working directory function call
	else if(tokens[0] == "mypwd"){
		mypwd();
	}
	// exit program
	else if(tokens[0] == "myexit" || cin.eof() == 1){
		return false;
	}
	// other specified internal function calls
	// mytree function call
	else if(tokens[0] == "mytree"){
		printTree(tokens);
	}
	// mytime function call
	else if(tokens[0] == "mytime"){
		myTime(tokens);
	}
	// mymtimes function call
	else if(tokens[0] == "mymtimes"){
		mymTime(tokens);
	}
	// mytimeout function call
	else if(tokens[0] == "mytimeout"){
		myTimeout(tokens);
	}
	// anything else is external function calls
	else{
		// make sure external command is a valid command
		if(tokens[0].find('/') == -1){
			tokens[0] = find_command(tokens[0]);
		}
		// process external command
		run_external_command(tokens[0], tokens);
	}
	
	return true;
}

// process pipes
void processPiping(vector<string> &tokens){
    vector<string> command1, command2, command3;

    int i = 0;
    while(tokens[i] != "|"){
        command1.push_back(tokens[i]);
        i++;
    }
    i++;

    while(i < tokens.size() && tokens[i] != "|"){
        command2.push_back(tokens[i]);
        i++;
    }

    if (i < tokens.size()) {
        i++;
        while(i < tokens.size()){
            command3.push_back(tokens[i]);
            i++;
        }
    }


    int pid1, pid2, pid3 = 0;
    int file_descriptor[2];
    int file_descriptor2[2];

    pipe(file_descriptor);

    if (command3.size() > 0){
        pipe(file_descriptor2);
    }

    pid1 = fork();
    if(pid1 == 0){
        if(command3.size() > 0){
            close(file_descriptor2[0]);
            close(file_descriptor2[1]);
        }
        close(file_descriptor[0]);
        close(STDOUT_FILENO);
        dup(file_descriptor[1]);
        close(file_descriptor[1]);

        execute_command(command1);
        exit(0);
    }

    pid2 = fork();
    if(pid2 == 0){
        if (command3.size() > 0){
            close(file_descriptor2[0]);
            close(STDOUT_FILENO);
            dup(file_descriptor2[1]);
            close(file_descriptor2[1]);
        }
        close(file_descriptor[1]);
        close(STDIN_FILENO);
        dup(file_descriptor[0]);
        close(file_descriptor[0]);

        execute_command(command2);
        exit(0);
    }

    if (command3.size() > 0){
        pid3 = fork();
        if(pid3 == 0){
            close(file_descriptor[0]);
            close(file_descriptor[1]);
            close(file_descriptor2[1]);
            close(STDIN_FILENO);
            dup(file_descriptor2[0]);
            close(file_descriptor2[0]);

            execute_command(command3);
            exit(0);
        }
    }

    close(file_descriptor[0]);
    close(file_descriptor[1]);

    if(command3.size() > 0){
        close(file_descriptor2[0]);
        close(file_descriptor2[1]);
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    if(command3.size() > 0){
        waitpid(pid3, NULL, 0);
    }
}



// process any IO redirects in the command
void IORedirect(vector<string> &tokens) {
    int inputRedir = -1, outputRedir = -1;
    int infile = -2, outfile = -2;

    for (int i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "<") {
            inputRedir = i;
            infile = open(tokens[i + 1].c_str(), O_RDONLY);
        } else if (tokens[i] == ">") {
            outputRedir = i;
            outfile = open(tokens[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        }
    }

    if (inputRedir != -1) {
        tokens.erase(tokens.begin() + inputRedir, tokens.begin() + inputRedir + 2);
    }
    if (outputRedir != -1) {
        tokens.erase(tokens.begin() + outputRedir, tokens.begin() + outputRedir + 2);
    }

    pid_t pid = fork();

    if (pid == 0) {
        if (inputRedir != -1) {
            dup2(infile, STDIN_FILENO);
            close(infile);
        }

        if (outputRedir != -1) {
            dup2(outfile, STDOUT_FILENO);
            close(outfile);
        }

        execute_command(tokens);
        exit(0);
    } else {
        if (outputRedir != -1) {
            close(outfile);
        }
        if (inputRedir != -1) {
            close(infile);
        }
        waitpid(pid, NULL, 0);
    }
}





// run all other external commands 
// void run_external_command(string command, vector<string> tokens){

	// int exec = 0;
	// int pid = fork();
	
	// char **args;
	// args = new char*[tokens.size()+1];
	// args[tokens.size()] = NULL;
	
	// // grab tokens
	// for(int i = 0; i < tokens.size(); i++){
		// // create memory for args array
		// args[i] = (char*) malloc(tokens[i].size() + 1);
		// strcpy(args[i], tokens[i].c_str());
	// }
	
	// // child process
	// if(pid == 0){
		// exec = execv(command.c_str(), args); 
		// cout << "Failed to run command.\n";
	// }
	// // parent
	// else{
		// // wait for child process
		// waitpid(pid, NULL, 0);
	// }
	
	// // deallocate memory for args
	// for(int i = 0; i < tokens.size(); i++){
		// free(args[i]);
	// }
	
	// // clear args
	// delete args;
// }

void run_external_command(const string& command, const vector<string>& args) {
    vector<char*> c_args;
    for (const auto& arg : args) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    if (execvp(command.c_str(), c_args.data()) == -1) {
        perror("Failed to run command");
        exit(1);
    }
}



// check for external commands
string find_command(string external_command){


	string env_cmd = getenv("PATH");
	
	if (env_cmd.empty()) {
		cerr << "PATH environment variable is not set or empty. Cannot search for external commands." << endl;
		return "";
	}
	string currentCommand = "";
	
	vector<string> commandTokens;
	getTokens(env_cmd, commandTokens, ':');
	
	for (int i = 0; i < commandTokens.size(); i++){
		
		//check the current command
		currentCommand = commandTokens[i];
		
		// add data to find the command's path
		currentCommand.push_back('/');
		currentCommand.append(external_command);
		
		
		// check to see if the command exists
		if(access(currentCommand.c_str(), F_OK) == 0){
			return currentCommand;			
		}
	}
	env_cmd = "";
	return env_cmd;		
}

// change directory 
void mycd(vector<string> tokens){
	
	// keep in same directory if none specified
	if(tokens.size() == 1){
		tokens.push_back(".");
	}
	string path = tokens[1];
	DIR *dir = opendir(path.c_str());
	int temp = chdir(path.c_str());
	
    if(temp == 0){
		cout << "Succesfully changed directory.\n";
        return;
    } 
	else{
        cout << "mytoolkit: mycd: " << dir << ": No such directory" << endl;
    }
}

// print the working directory
void mypwd(){
	
	char buffer[200];
    if(getcwd(buffer, 200) != nullptr){
        cout << buffer << '\n';
    } 
	else{
        cout << "Error getting current working directory.\n";
    }
}

// process the tokens for the tree command
void printTree(vector<string> tokens){
	
	
	DIR *directory;
	
	if(tokens.size() == 1){
		tokens.push_back(".");
	}
	// checking validity of tokens for directory
	else if(tokens.size() > 2){
		cout << "Too many arguments.\n";
		return;
	}
	
	// set the directory path
	directory = opendir(tokens[1].c_str());
	errno = 0;
	
	// check for valid directory
	if(directory == NULL && errno == ENOENT){
		cout << "Directory doesn't exist.\n";
		closedir(directory);
		return;
	}
	else if(directory == NULL && errno == ENOTDIR){
		cout << "Not a directory.\n";
		return;
		closedir(directory);
	}
	
	// close the directory
	closedir(directory);
	cout <<  "Printing directory as tree: " << tokens[1] << endl;
	
	// print the desired directory as a tree
	navigate(tokens[1], ".");
	
}

// print the directory passed in tree format
void navigate(string start, string path){
    // Print current directory/file
    cout << start << "| - - -" << path << endl;

    // Check if path is a directory
    DIR* directory = opendir(path.c_str());
    if(directory != NULL) {
        start += "  ";
        // Read all the entries in the directory
        struct dirent* entries;
        while((entries = readdir(directory)) != NULL){
			//check for hidden files
            if(entries->d_name[0] == '.'){
				// Skip hidden files
                continue;  
            }
			
            string entry_path = path + "/" + entries->d_name;
			
            struct stat info;
			
			//check validity
            if(stat(entry_path.c_str(), &info) != 0){
				// Skip if stat() fails
                continue;  
            }
			//check if entry is a directory
            if(S_ISDIR(info.st_mode)){
				// Recurse into subdirectory
                navigate(start, entry_path);  
            } 
			// Print file
			else{
                cout << start << "| - - -" << entries->d_name << endl;  
            }
        }
        closedir(directory);
    }
}

// internal funciton myTime
void myTime(vector<string> &tokens){
	
	// clock init
	clock_t clockStart, clockEnd;
	double systemClock = sysconf(_SC_CLK_TCK);
	
	
	// time calculations
	tms start, end;
	float cpu, systemTime, total = 0;
	
	
	//check for valid command to run
	if(tokens.size() < 2){
		cout << "Failed to run command.\n";
		return;
	}
	
	//remove token
	tokens.erase(tokens.begin());
	
	// start the timer
	clockStart = times(&start);
	
	// fork the command 
	int pid = fork();
	if(pid == 0){
		processCommands(tokens);
		exit(0);
	}
	// parent process 
	else{
		// wait for child
		waitpid(pid, NULL, 0);
		
		// end the clock once process is over
		clockEnd = times(&end);
		
		// calculate user cpu time system time and total time
		cpu = (end.tms_cutime - end.tms_utime) - (start.tms_cutime - start.tms_utime);
		systemTime = (end.tms_cstime - end.tms_stime) - (start.tms_cstime - start.tms_stime);
		
		
		total = clockEnd - clockStart;
		
		// convert to minutes and seconds for user CPU time
		cout << "\nUser CPU time: \t" << cpu << "ms\n";
		
		// convert to minutes and seconds for system time
		cout << "System time: \t" << systemTime << "ms\n";
		
		// convert to minutes and seconds for total elapsed time
		cout << "Total time elapsed: \t" << total << "ms\n";
		
		// adjust values 
		cpu = cpu/systemClock;
		systemTime = systemTime/systemClock;
		total = total/systemClock;
		
		
	}
	return;
}

// process mymtime command tokens 
void mymTime(vector<string> tokens){
	
	// check for valid mymtime command
	if(tokens.size() == 1){
		tokens.push_back(".");
	}
	else if(tokens.size() > 2){
		cout << "Invalid command.\n";
		// break from function
		return;
	}
	
	// directory
	DIR *dir;
	
	// try to open the directory 
	dir = opendir(tokens[1].c_str());
	errno = 0;
	
	// check for valid directory from open
	if(dir == NULL && errno == ENOENT){
		cout << "Could not find directory.\n";
		closedir(dir);
		return;
	}
	else if(dir == NULL && errno == ENOTDIR){
		cout << "Invalid directory.\n";
		return;
		closedir(dir);
	}
	
	// close the directory
	closedir(dir);
	
	// hours of the day
	vector<int> hours(24, 0);
	
	// time 
	time_t t;
	time(&t);
	
	countTimes(tokens[1], t, hours);
	
	// Print the results
	cout << "Files modified in the last 24 hours:" << endl;
	for (int i = 0; i < hours.size(); i++) {
		cout << i << " hour(s) ago: " << hours[i] << " file(s)" << endl;
	}
}


// count the files modified at each time
void countTimes(string path, time_t now, vector<int> &hours){
	// get and open the directory
    DIR *dir = opendir(path.c_str());
	
	//check for empty directory
    if(dir == NULL){
        cout << "Directory could not be opened: " << path << endl;
        return;
    }

	// pull each of the entries from the directory
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        string filename = entry->d_name;
		
		//skip hidden files
        if(filename == "." || filename == ".."){
            continue;
        }
		
		//other invalid files
        string full_path = path + "/" + filename;
        struct stat stat_buf;
        if(stat(full_path.c_str(), &stat_buf) == -1){
            cout << "Failed to get status: " << full_path << endl;
            continue;
        }
		
		//pull data and look when modified if successful
        if(S_ISREG(stat_buf.st_mode)){
            time_t mtime = stat_buf.st_mtime;
            if(mtime >= now - 24 * 60 * 60){
				//calculate the hour modified
                int hour = (now - mtime) / (60 * 60);
                if(hour < 24){
                    hours[hour]++;
                }
            }
        } 
		else if(S_ISDIR(stat_buf.st_mode)){
            countTimes(full_path, now, hours);
        }
    }

	//close the directory
    closedir(dir);
}


void myTimeout(vector<string>& tokens){
    // Fork a new process
    pid_t pid = fork();
	
	int snds = stoi(tokens[1]);


    if(pid == 0){
        // Child process
        vector<char*> args;
        for(const auto& arg : tokens){
            args.push_back(const_cast<char*>(arg.c_str()));
        }
        args.push_back(nullptr);

        // Execute the command
        execvp(args[0], args.data());
        exit(0);
    } 
	else if(pid > 0){
        // Parent process
        int status = 0;
        pid_t wait_result = 0;

        // Set up a timeout using alarm
        alarm(snds);

        // Wait for the child process to finish or for the timeout
        while((wait_result = waitpid(pid, &status, 0)) > 0){
            // If child terminated normally or by a signal, break the loop
            if(WIFEXITED(status) || WIFSIGNALED(status)){
                break;
            }
        }

        // If the timeout occurred and the child process is still running
        if(wait_result == 0){
            // Send the TERM signal to the child process
            kill(pid, SIGTERM);
        }
    } 
	else{
        // Fork failed
        cerr << "Error: fork() failed." << endl;
    }
}



// main function
int main(){
	
	// initialize user input
	string input = "";
	vector<string> tokens;
	bool running;
	
	// program runs until terminated
	while(1){
		
		// should print $ and space like specified
		cout << "$ ";
		
		// grab the line entered and tokenize it
		getline(cin, input, '\n');		
		getTokens(input, tokens, ' ');
		
		if(!processCommands(tokens)){
			break;
		}
	}
	return 0;	
}
