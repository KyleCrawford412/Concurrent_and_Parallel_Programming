#include <iostream>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace std;

// count the files modified at each hour
void count_files(const string &path, time_t now, vector<int> &counts){
	
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
                    counts[hour]++;
                }
            }
        } 
		else if(S_ISDIR(stat_buf.st_mode)){
            count_files(full_path, now, counts);
        }
    }

	//close the directory
    closedir(dir);
}

int main(int argc, char *argv[]) {
    string path = ".";
    if (argc > 1) {
        path = argv[1];
    }

    time_t now = time(NULL);
	
	//vector to store number of files within each hour modified.
    vector<int> counts(24, 0);
	
	// call function to count the files in each time slot
    count_files(path, now, counts);

	//print out results
    for (int i = 0; i < 24; i++) {
        cout << i << ":00 - " << i + 1 << ":00: " << counts[i] << " files" << endl;
    }

    return 0;
}
