#include <iostream>
#include <cstring>
#include <chrono>

#include <vector>
#include <cstdlib>

#include <sys/wait.h>
#include <unistd.h>


using namespace std;

int main(int num, char *text[]) {
    if(num < 2) {
        cout << "Usage: " << text[0] << " cmd [arguments]";
        return 1;
    }

	// start the clock for the command
    auto start_time = chrono::high_resolution_clock::now();

    vector<char *> cmd_args;
	
    for(int i = 1; i < num; i++){
        cmd_args.push_back(text[i]);
    }
	
    cmd_args.push_back(NULL);

	//run command in separate process
    pid_t pid = fork();
	
	//check to see if command worked
    if(pid == -1){
		cout << "Failed to fork";
        return 1;
    } 
	else if(pid == 0) {
        if(execvp(cmd_args[0], cmd_args.data()) == -1){
            return 1;
        }
    } 
	else{
        int status;
        waitpid(pid, &status, 0);
		
		// stop the clock after command has run
        auto end_time = chrono::high_resolution_clock::now();
		
		// calculate the time elapsed
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

		// print time
        cout << "Time to run command: " << elapsed.count() << " ms" << endl;
    }

    return 0;
}
