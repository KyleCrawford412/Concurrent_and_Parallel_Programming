
All of the specified functions were internal function calls like mycd, mypwd, mytime etc. This is because we already had the base of the functions to work in this project. That is why 
I made specific functions for each one that were called when the respective command was detected in the token string. Anything else was processed as an external command.
The external commands were processed by checking for a valid command by looking for a path to it and then executing it.
The program only supportstwo pipes in a command line as given in the example for the project. 
You can run the program with make which creates an executable mytoolkit. 