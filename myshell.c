/*
	Name:  Augustine Nguyen

*/

#define  _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <linux/limits.h>
#include <ctype.h>

#define CMD_MAX 255
#define ARGU_MAX 12
#define WHITE_SPACE " \n\t\r"
#define HIST_MAX 15
#define PID_MAX 15

//function that allows for !history to work via recursion
//takes in tokens and history strings for command processing
void CheckCommand(char * cmd_tok[ARGU_MAX], char * cmd_hist[HIST_MAX]);

//global variables for keeping track of pid history
int pid_count = 0;
pid_t pidID[PID_MAX];

int main(void)
{
	size_t max_cmd_size = CMD_MAX;
	size_t cmd_size;
	char * cmd_str = (char *) malloc(max_cmd_size * sizeof(char));
	char * cmd_tok[ARGU_MAX];
	char * tok;

	char * realloc_ptr;

	int status;

	char * cmd_hist[HIST_MAX];

	int errnum;

	int j;
	int i;
	int hist_count = 0;

	//emptying token and history arrays before use
	for (i=0; i<ARGU_MAX; i++)
	{
		cmd_tok[i] = NULL;
	}

	for (i=0; i<HIST_MAX; i++)
	{
		cmd_hist[i] = NULL;
	}

	//main loop
	while (1)
	{
		printf("msh> ");

		//uses getline for inputting stdin
		cmd_size = getline(&cmd_str, &max_cmd_size, stdin);

		//counts the amount of spaces to fullfill requirement 6
		//j is the amount of spaces in each cmd_str
		i=0;
		j=0;
		while (cmd_str[i])
		{
			if (isspace(cmd_str[i++])) j++;
		}

		//typing in quit or exit will allow user to exit program
		//breaking out of loop means exiting successfully
		if ((strcmp(cmd_str, "quit\n") == 0) || (strcmp(cmd_str, "exit\n") == 0)) break;

		//if the amount of spaces matches the size of the cmd_size (given to use by getline)
		//then nothing happens
		if (j != cmd_size)
		{
			//for keeping track of History

			//to make sure ! commands are not in History
			//leaving ! in history means possible infinite recursion
			if (cmd_str[0] != '!')
			{
				//if the history counter has not reached the max yet
				//cmd_hist will have the next empty index being filled
				if (hist_count < HIST_MAX)
				{
					//parses out any \n or \r in the history string
					tok = NULL;
					tok = strtok(cmd_str, "\n\r");

					//fills cmd_hist up
					//the % is very unneccessary as hist_count never goes about the max in this block
					//hist_count by itself would be sufficient
					cmd_hist[(hist_count) % HIST_MAX] = malloc(sizeof(char) * strlen(tok));
					strcpy(cmd_hist[(hist_count++) % HIST_MAX], tok);

				}
				//when the limit is reached, the array is filled via a "left sliding" array structure (idk the name of it, I made it up)
				//all elements of the array are slid down the index, leaving zero index erased and last index to be a duplicate
				//then the duplicate is replaced by the newest value
				/*
					example:  old - a,b,c,d,e,f
							  slide - b,c,d,e,f,f
							  replace - g
							  new - b,c,d,e,f,g
				*/
				else
				{
					//sliding
					i=0;
					while (i+1 < HIST_MAX)
					{
						realloc_ptr = realloc(cmd_hist[i], sizeof(char) * strlen(cmd_hist[i+1]));
						strcpy(cmd_hist[i], cmd_hist[i+1]);
						i++;
					}

					//last index replaced
					tok = NULL;
					tok = strtok(cmd_str, "\n\r");
					realloc_ptr = realloc(cmd_hist[HIST_MAX-1], sizeof(char) * strlen(tok));
					strcpy(cmd_hist[HIST_MAX-1], tok);
				}
			}


			//checking if cmd is over 255 characters
			if(cmd_size > CMD_MAX)
			{
				printf("\nCommand Exceeds 255 Characters\n");
			}

			//processing cmd_str into cmd_tok's
			tok = strtok(cmd_str, WHITE_SPACE);
			cmd_tok[0] = tok;
			i=1;
			j=0;
			while (tok != NULL)
			{
				tok = strtok(NULL, WHITE_SPACE);
				cmd_tok[i++] = tok;
			}
			//call function for cmd processing
			CheckCommand(cmd_tok, cmd_hist);
		}

	}

	free(cmd_str);
	return 0;
}

//function that allows for !history to work via recursion
//takes in tokens and history strings for command processing
void CheckCommand(char * cmd_tok[ARGU_MAX], char * cmd_hist[HIST_MAX])
{
	int i, j, k, err, first_hist, last_hist;
	pid_t child;
	int status;
	int errnum;
	char * tok;
	int index_hist;

	//for pipe
	int fs[2], errval;

	//changes directory
	if (strcmp(cmd_tok[0], "cd") == 0)
	{
		if (chdir(cmd_tok[1]) == -1) perror("Error");
	}
	//prints history of last 15 commands
	//note:  commands can be failed or successful, the only things that isn't in history are commands starting with !
	else if (strcmp(cmd_tok[0], "history") == 0)
	{
		i=0;
		while ((i < HIST_MAX) && (cmd_hist[i] != NULL))
		{
			printf("%d:  %s\n", i, cmd_hist[i]);
			i++;
		}
	}
	//reruns old commands via recursion
	//first if statement checks if the fisrt char is ! and the second char isn't a space, tab, or next line
	else if ((cmd_tok[0][0] == '!') && (cmd_tok[0][1] != 0) && !isspace(cmd_tok[0][1]))
	{
		//if there is a second string, the ! cmd fails
		if (cmd_tok[1] != NULL)
		{
			printf("Error: Too Many Arguments\n");
		}
		//first makes sure that the ! command is done on the proper index and then it makes sure the value at that index is valid
		//once we know the value is valid, we have to tokenize it by reusing cmd_tok and utilizing strtok
		//once all that is done with, the new cmd_tok and old cmd_hist is passed into CheckCommand for recursion
		else
		{
			//gets rid of the ! and white space on the !cmd
			//it is turned into an int and stored in index_hist
			tok = strtok(cmd_tok[0], " !\n\r\t");
			index_hist = atoi(tok);

			//checks bounds
			if (index_hist < 15)
			{
				//checks if valid
				if ((cmd_hist[index_hist] != NULL))
				{
					//empties cmd_tok
					tok = NULL;
					for (i=0; i<ARGU_MAX; i++)
					{
						cmd_tok[i] = NULL;
					}

					//tokenizes cmd_hist and fills cmd_tok up
					tok = strtok(cmd_hist[index_hist], WHITE_SPACE);
					cmd_tok[0] = tok;
					i=1;
					j=0;
					while (tok != NULL)
					{
						tok = strtok(NULL, WHITE_SPACE);
						cmd_tok[i++] = tok;
					}
					//recurse
					CheckCommand(cmd_tok, cmd_hist);
				}
				else
				{
					printf("Error:  Command Not in History\n");

				}
			}
			else
			{
				printf("Error:  Command Out of Bounds\n");
			}
		}
	}
	//prints out last 15 pid's
	//note:  it doesn't count invalid command entries
	//BUT it does count invalid parameters
	//example:  "ls -w" would count, but "a;sdjf" would not
	else if (strcmp(cmd_tok[0], "pidhistory") == 0)
	{
		//limit hasn't been reached, prints to pid_count
		if (pid_count < PID_MAX)
		{
			i=0;
			while (i<pid_count)
			{
				printf("%d: %d\n", i, (int) pidID[i]);
				i++;
			}
		}
		//limit reached, counts to max
		else
		{
			i=0;
			while (i<PID_MAX)
			{
				printf("%d: %d\n", i, (int) pidID[i]);
				i++;
			}
		}
	}
	//executes real commands found in files
	//the execution of commands is simple:  fork, parent waits, child runs exec.
	/*
		keeping track of pidhistory is a lot more complicated:
		-the block is divided into two parts--pid_count under max and pid_count exceeds max
			-under max just inserts new pid's into the array normally
			-exceeds max pidID utilizes a left and right sliding array
				-if the command is valid, then the array is slid left and a new pid is inserted into the last elements
				-otherwise, the array is slid back to the right and the previous element for index 0 is placed back into the array (the array is slid left by default)
					-this means the array stays the same as before the invalid cmd
					-more details on right sliding:
						-pidID's first element is saved prior to sliding,
						-once elements are slid to the right,
						-the last element is erased and first element's dup is replaced with the saved elements
						ex:	old - b,c,d,e,f,g
							slid - b,b,c,d,e,f
							insert - a,b,c,d,e,f

		preventing invalid command pid's to enter pidhistory is like this:
		-open a pip on fs[2]
		-if in the child, close reading end before exec is called, and after exec, write to it an error value
			-this means if the exec was successful, then the write message wouldn't be able to go through
		-if in the parent close, writing end, read in the error value, then close reading end.
			-if the value is not 0, that means the exec was unsuccessful
				-if we're unsuccessful and pid_count is maxed out,
					-slide pidID array to the right, replace first element with previously saved element
						-restoring array from before the left slide
					-then subtract 1 from pid_count
				-if we're unsuccessful and pid_count isn't maxed
					-simply set the previously inserted element to be zero and then subtract 1 from pid_count
	*/
	else
	{
		//opens pipe
		pipe(fs);

		//pid_count under max
		if (pid_count < PID_MAX)
		{
			//store pid and fork, child exec's or writes if unsuccessful, parent reads and if not 0 restores array and then waits.
			pidID[pid_count % PID_MAX] = fork();
			if (pidID[pid_count % PID_MAX] == 0)
			{
				close(fs[0]);
				if (execvp(cmd_tok[0], cmd_tok) == -1) //perror("Error ");
					{
						errnum = errno;
						if (errnum == 2)
							printf("Error: Command Not Found\n");
						else
							perror("Error: ");
						write(fs[1], &errno, sizeof(errno));
					}
					exit(EXIT_SUCCESS);
			}
			else
			{
				close(fs[1]);
				k = read(fs[0], &err, sizeof(err));
				close(fs[0]);

				//if exec is unsuccessful
				if (k != 0)
				{
					pidID[pid_count % PID_MAX] = 0;
					pid_count--;
				}
			}
			waitpid(pidID[pid_count % PID_MAX], &status, 0);
		}
		//pid_count exceeds max
		else
		{
			//store first element for potential array restoration
			first_hist = pidID[0];

			//slides left by default
			i=0;
			while (i+1 < PID_MAX)
			{
				pidID[i] = pidID[i+1];
				i++;
			}

			//store pid and fork, child exec's or writes if unsuccessful, parent reads and if not 0 restores array and then waits.
			pidID[PID_MAX-1] = fork();
			if (pidID[PID_MAX-1] == 0)
			{
				close(fs[0]);
				if (execvp(cmd_tok[0], cmd_tok) == -1) //perror("Error ");
					{
						errnum = errno;
						if (errnum == 2)
							printf("Error: Command Not Found\n");
						else
							perror("Error: ");
						write(fs[1], &errno, sizeof(errno));
					}
					exit(EXIT_SUCCESS);
			}
			else
			{
				close(fs[1]);
				k = read(fs[0], &err, sizeof(err));
				close(fs[0]);
				//if exec is unsuccessful
				if (k != 0)
				{
					//slide right
					i = PID_MAX-1;
					while ((i-1) >= 0)
					{
						pidID[i] = pidID[i-1];
						i--;
					}

					//restore first element
					pidID[0] = first_hist;

					pid_count--;
				}
			}

			waitpid(pidID[PID_MAX-1], &status, 0);
		}
		pid_count++;
	}
}
