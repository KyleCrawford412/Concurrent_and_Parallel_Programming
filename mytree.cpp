//Kyle Crawford
//Concurrent and Parallel Programming
//Spring 2023

//Print all the files and subdirectories in a tree structure

//Includes to navigate directories
#include <iostream>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <algorithm>


using namespace std;

void navigate(string start, string path) {
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


int main(int argc, char* argv[]) {
    string start_path = ".";
    if (argc == 2) {
        start_path = argv[1];
    }
	//pass in directory to work from
    navigate("", start_path);
    return 0;
}
