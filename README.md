# C Command Shell

### Description
- C program that implements most of the functionality of a linux bash shell
- Most of the program is input processing, using getline(), strtok()
  - Then getting the program to understand the processed strings, mostly if-else statements with strcmp
- Utilises mainly fork() and exec() functions to execute command input from user
  - Additionally uses pipe() close() read() write() for the catching errors on exec() runs

### Commands
- Supports execution of files in the current directory as well as /usr/local/bin, /usr/bin, /bin
- Non-exec commands:
  - "pidhistory" - pulls up history of the last 15 pid's ran using exec
    - note: doesn't count failed executions, but it does count executions with the wrong arguments
    - example:  "ls -W" will count int pidhistory but "alsdjf;" won't, if alsdjf; isn't an executable
  - "history" - pulls up last 15 entries in the command line and gives it an index between 0-14, valid or not
    - doesn't count commands starting with ! or empty space
  - "![history index]" - reruns the entry on the command line of that particular index, cannot run commands outside 0-14

### Running it
- Ran on a kali-linux VM and the uta omega servers
  - Should run on most linux-based system
- To run: "gcc -o myshell myshell.c -std=c99" then "./myshell"
