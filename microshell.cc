#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h> //прототипы системных вызовов
#include <sys/types.h> //типы системных вызовов
#include <sys/stat.h>
#include <sys/wait.h>

#include <string.h>
#include <errno.h>

#include <iostream>
//~ #include <cstring>
#include <string>
#include <vector>

#include <readline/readline.h> 
#include <readline/history.h> 

using namespace std;

string HOME_DIR;
string CURR_DIR;

// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J") 

void init_microsha() 
{ 
    clear(); 
    printf("\n\n\n\n******************"
        "************************"); 
    printf("\n\n\n\t****MICROSHA****");  
    printf("\n\n\n\n*******************"
        "***********************"); 
    char* username = getenv("USER"); 
    printf("\n\n\nUSER is: @%s", username); 
    printf("\n"); 
    sleep(1); 
    clear(); 
}

void greeting_f(string *s) {
	char cwd[1000];
	getcwd(cwd, sizeof(cwd));
	uid_t uid = getuid();
	if ((long long) uid == 0) {
		printf("%s! ", cwd);
	} else {
		printf("%s> ", cwd);
	}
	CURR_DIR = string(cwd);
	char *buffer;
    size_t bufsize = 32;
    buffer = (char *)malloc(bufsize * sizeof(char));
    if(buffer == NULL) {
        perror("Sorry, unable to allocate buffer");
        exit(1);
    }
    getline(&buffer,&bufsize,stdin);
	*(s) = (string) buffer;
	free(buffer);
}

vector<string> split(const string& s)
{
    vector<string> ret;
    typedef string::size_type string_size;
    string_size i = 0;
    while (i != s.size()) {
        while (i != s.size() && isspace(s[i]))
            ++i;
        string_size j = i;
	while (j != s.size() && !isspace(s[j]))
		j++; 
	if (i != j) {
		ret.push_back(s.substr(i, j - i));
		i = j;
	}
    }
    return ret;
}

void peter_piper (vector <vector <string> > commands){
	int i; 
	int n_commands = commands.size();
	for(i = 0; i < n_commands - 1; i++){
		int fd[2];	
		pipe(fd);
		pid_t pid = fork();
		if (pid == 0){
			dup2(fd[1], 1);
			close(fd[0]);
			vector<const char *> v;
			for (vector<string>::size_type j = 0; j < commands[i].size(); j++) {
				const char *tmp = commands[i][j].c_str();
				v.push_back(tmp);
			}
			v.push_back(NULL);		
			/*int status = */
			execvp(v[0], (char * const*) &v[0]);
			perror("Failed to do execvp!\n");
			/*exit(status);*/
		} else {
			dup2(fd[0], 0);
			close(fd[1]);
		}	
	}
	vector<const char *> last_command;
	for (vector<string>::size_type j = 0; j < commands[n_commands-1].size(); j++) {
		const char *tmp = commands[n_commands-1][j].c_str();
		last_command.push_back(tmp);
	}
	last_command.push_back(NULL);
	execvp(last_command[0], (char * const*) &last_command[0]);
	//int status = //
	perror("Failed to do execvp!\n");
	// exit(status); //
}

int main(int argc, char **argv, char **envp) {
	
	init_microsha();
	
	//printf("INITIALIZED\n");
	
	// getting shell's home directory
	char *buffer_for_home_dir;
	buffer_for_home_dir = getenv("HOME");
	HOME_DIR = (string) buffer_for_home_dir;
	
	//printf("GOT HOME DIRECTORY\n");
	
	// here is the programme per se
	while (1) {
		
	// let's greet the user and get his command line
		string s;
		greeting_f(&s);
		
	// let's parse this line into commands
		vector<string> v = split(s);
		
	// let's create a two-dimensional array	
		vector < vector < string > > commands;
		int flag = 0;
		int i = 0;
		int n_words = v.size();
		while (1) {
			vector<string> row;
			while (1) {
				if (i == n_words) {
					//row.push_back(NULL);
					flag = 1;
					break;
				} else if (v[i] == "|") {
					//row.push_back(NULL);
					i++;
					break;		
				} else {
					row.push_back(v[i]);
					i++;	
				}
			}
			commands.push_back(row);
			if (flag) break;
		}

// CASE 1. NO PIPES
		
		if (commands.size() == 1) {
			
// Commands "cd" and "pwd" are very special. In the other cases we will use run_command function.	

			if (commands[0][0] == "cd") {
				if (commands[0].size() == 1) {
					chdir(HOME_DIR.c_str());
				} else {
					if (chdir(commands[0][1].c_str()) < 0) {
						perror("PATH DOES NOT EXIST!");
					}
				}
			} else if (commands[0][0] == "pwd") {
				printf("%s\n", CURR_DIR.c_str());
			} else if (commands[0][0] == "exit") {
				printf("\nSee you soon :)\n");
				exit(0);
			} else if (commands[0][0] == "help") {
				printf("\n\n\nIT IS NOT A REAL BASH, JUST A STUPID EMULATOR\n\tPLEASE EXIT AND STOP WASTING YOUR TIME\n");						
			} else {
				
// All the other commands, which may contain '>' or '<'
		
				int n_words = commands[0].size();
				string output_file_name;
				string input_file_name;
				int flag_more = 0;
				int flag_less = 0;
				for (int i = 0; i < n_words - 1; i++) {
					if (commands[0][i] == ">") {
						output_file_name = commands[0][i+1];
						flag_more = 1;
						if ((output_file_name == ">") or (output_file_name == "<")) {
							perror("A filename is missed :(\n");
						}
					}
					if (commands[0][i] == "<") {
						input_file_name = commands[0][i+1];
						flag_less = 1;
						if ((input_file_name == ">") or (input_file_name == "<")) {
							perror("A filename is missed :(\n");
						}				
					}
				}
				vector<const char*> command; 
				for (vector<string>::size_type j = 0; j < commands[0].size(); j++) {
					if ((commands[0][j] == "<") or (commands[0][j] == ">")) break;
					const char *tmp = commands[0][j].c_str();
					command.push_back(tmp);
				}
				command.push_back(NULL);

				pid_t pid = fork();

				if (pid == 0) {					
					if (flag_less) {
						int in_fd = open((const char*) input_file_name.c_str(), O_RDONLY | O_CREAT, 0666);
						dup2(in_fd, 0);
					}
					if (flag_more) {	
						int out_fd = open((const char*) output_file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
						dup2(out_fd, 1);
					}
					execvp(command[0], (char * const*)&command[0]);
					perror("run_command");
				} else {
					int code;
					wait(&code);
				}
			}
		} else if (commands.size() > 1) {			
// CASE 2. PIPES	
			pid_t pid = fork();
			
			if (pid == 0) {						
				peter_piper(commands);			
			} else {
				int code;
				wait(&code);
			}			
		}				
	}
	return 0;
}








