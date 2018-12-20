#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> // prototypes of the system calls
#include <sys/types.h> // types of the system calls
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <vector>
#include <glob.h> // the one which works with regular expressions
#include <sys/times.h>

using namespace std;

string HOME_DIR;
string CURR_DIR;

// The command line user wrote may contain regular expressions with * or |
// To help the microshell deal with such cases my_glob function was written
vector<string> my_glob(const string &pattern) {
    glob_t glob_result;
    vector<string> filenames;
    // do the glob operation
    int return_value = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
    if(return_value != 0) {
        globfree(&glob_result);
        //perror("metasymbol function failed\n");											//
        filenames.push_back(pattern);
		return filenames;
    }
    // collect all the filenames into a vector
    //vector<string> filenames;
    for(size_t i = 0; i < glob_result.gl_pathc; ++i) {
        filenames.push_back(string(glob_result.gl_pathv[i]));
    }
    // let's clean the glob_result
    globfree(&glob_result);
    // done
    return filenames;
}
// Now, let's parse the user's command into the command itself,
// It's option flags and arguements
vector<string> split(const string& s) {
    vector<string> ret;
    typedef string::size_type string_size;
    string_size i = 0;

    // invariant: we have processed characters [original value of i, i) 
    while (i != s.size()) {
       // ignore leading blanks
       // invariant: characters in range [original i, current i) are all spaces
        while (i != s.size() && isspace(s[i]))
            ++i;
       // find end of next word
        string_size j = i;
       // invariant: none of the characters in range [original j, current j)is a space
		while (j != s.size() && !isspace(s[j]))
			j++;
      // if we found some nonwhitespace characters 
		if (i != j) {
         // copy from s starting at i and taking j - i chars
			ret.push_back(s.substr(i, j - i));
			i = j;
		}
    }
    // ...and dealing with metasymbols:
    vector<string> ret_meta;
	int N_ret = ret.size();
	int k;
	for(k = 0; k < N_ret; k++) {
		if ((ret[k].find('*') != std::string::npos) | (ret[k].find('?') != std::string::npos)) {
			vector<string> file_names = my_glob(ret[k]);
			int c;
			int N_files = file_names.size();
			for (c = 0; c < N_files; c++) {
				ret_meta.push_back(file_names[c]);
			}
		} else {
			ret_meta.push_back(ret[k]);
		}		
	}
    return ret_meta;
}

int check_f(vector <vector <string> > commands) {
	int n_commands = commands.size();
	int continue_flag = 0;
		
	int n_words = commands[0].size();
	int flag_more = 0;
	int flag_less = 0;
	int more_count = 0;
    int less_count = 0;
    for (int j = 0; j < n_words-1; j++) {
		if (commands[0][j] == ">") continue_flag = 1;
		if (commands[0][j] == "<") {
			less_count += 1;
		}
		if (less_count >= 2) continue_flag = 1;
	}
	if ((commands[0][0] == ">") || (commands[0][0] == "<")) {
		continue_flag = 1;
	}		
	if ((commands[0][n_words - 1] == ">") || (commands[0][n_words - 1] == "<")) {
		continue_flag = 1;
	}	
	
	for(int i = 1; i < n_commands - 1; i++){
		int n_words = commands[i].size();
        for (int j = 0; j < n_words - 1; j++) {
			if ((commands[i][j] == ">") || (commands[i][j] == "<")) {
				continue_flag = 1;
			}				
		}
	}
	
	n_words = commands[n_commands-1].size();
	flag_more = 0;
	flag_less = 0;
	more_count = 0;
    less_count = 0;
    for (int j = 0; j < n_words-1; j++) {
		if (commands[n_commands-1][j] == "<") continue_flag = 1;
		if (commands[n_commands-1][j] == ">") {
			more_count += 1;
		}
		if (more_count >= 2) continue_flag = 1;
	}
	if ((commands[n_commands-1][0] == ">") || (commands[n_commands-1][0] == "<")) {
		continue_flag = 1;
	}		
	if ((commands[n_commands-1][n_words - 1] == ">") || (commands[n_commands-1][n_words - 1] == "<")) {
		continue_flag = 1;
	}				
	return continue_flag;
}


// This function implements the pipeline


void peter_piper (vector <vector <string> > commands){
	int i; 
	int n_commands = commands.size();
	for(i = 0; i < n_commands - 1; i++){
		int fd[2];	
		pipe(fd);
		pid_t pid = fork();
		if (pid == 0){
			signal(2, SIG_DFL);
			dup2(fd[1], 1);
			close(fd[0]);
			vector<const char *> v;
			for (vector<string>::size_type j = 0; j < commands[i].size(); j++) {
				if (commands[i][j] == "<") {
					string input_file_name = commands[i][j + 1];
					int in_fd = open((const char*) input_file_name.c_str(), O_RDONLY | O_CREAT, 0666);
					dup2(in_fd, 0);
					break;
				}
				const char *tmp = commands[i][j].c_str();
				v.push_back(tmp);
			}
			v.push_back(NULL);		
			int status = execvp(v[0], (char * const*) &v[0]);
            if (status != 0) printf("Please make sure your command is correct\n");
			//~printf("Failed to do execvp!\n");
			exit(status);
		} else {
			dup2(fd[0], 0);
			close(fd[1]);
		}	
	}
//  The last command is different: we want it write to stdout
	vector<const char *> last_command;
	for (vector<string>::size_type j = 0; j < commands[n_commands-1].size(); j++) {
		if (commands[i][j] == ">") {
			string output_file_name = commands[i][j + 1];
			int out_fd = open((const char*) output_file_name.c_str(), O_WRONLY | O_CREAT, 0666);
			dup2(out_fd, 1);
			break;
		}
		const char *tmp = commands[n_commands-1][j].c_str();
		last_command.push_back(tmp);
	}
	last_command.push_back(NULL);
	int status = execvp(last_command[0], (char * const*) &last_command[0]);
    if (status != 0) printf("Please make sure your command is correct\n");
	exit(status);
}



int main(int argc, char **argv, char **envp) {
	signal(2, SIG_IGN);
	//init_microsha();
	
	// getting shell's home directory
	char *buffer_for_home_dir;
	buffer_for_home_dir = getenv("HOME");
	HOME_DIR = (string) buffer_for_home_dir;
	
	// here is the programme per se
	while (1) {
		
	// let's greet the user and get his command line
		string s;
		
		char cwd[1000];
		getcwd(cwd, sizeof(cwd));
		uid_t uid = getuid();
		if ((long long) uid == 0) {
			printf("%s! ", cwd);
		} else {
			printf("%s> ", cwd);
		}
		CURR_DIR = string(cwd);
						
		getline(cin, s);
		if (cin.eof() && (s.length() == 0)) {
			cout << endl;
			break;
		}
		if (cin.eof()) return 0;		
		
	// let's parse this line into commands
		vector<string> v = split(s);
		
		if (v.size() != 0) {
		
		// let's create a two-dimensional array	of commands
			vector < vector < string > > commands;
			int flag = 0;
			int i = 0;
			int n_words = v.size();
			while (1) {
				vector<string> row;
				while (1) {
					if (i == n_words) {
						flag = 1;
						break;
					} else if (v[i] == "|") {
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
			
	// It was written in the task that cases with and without '|' should be 
	// treated differently		

	// CASE 1. NO PIPES
			
			if (commands.size() == 1) {
				
	// Commands "cd", "pwd", "exit" (optional), "help" and "time"  are very special:	

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
					
				} else if (commands[0][0] == "time") {
					
					vector<const char*> command; 
					for (vector<string>::size_type j = 1; j < commands[0].size(); j++) {
						if ((commands[0][j] == "<") or (commands[0][j] == ">")) break;
						const char *tmp = commands[0][j].c_str();
						command.push_back(tmp);
					}
					command.push_back(NULL);				
					tms start;
					tms end;
					clock_t clock_start;
					clock_start = times(&start);
					pid_t pid = fork();
					if (pid == 0) {
						signal(2, SIG_DFL);
						int status = execvp(command[0], (char * const *)&command[0]);
                        if (status != 0) printf("Please make sure your command is correct\n");
						exit(status);
					}
					int code;
					wait(&code);
					clock_t clock_end;
					clock_end = times(&end);
					fprintf(stderr, "real\t%lf\nsys\t%lf\nuser\t%lf\n", 10000 * (double)(clock_end - clock_start) / CLOCKS_PER_SEC, 10000 * (double)(end.tms_cstime - start.tms_cstime) / CLOCKS_PER_SEC, 10000 * (double)(end.tms_cutime - start.tms_cutime) / CLOCKS_PER_SEC);
					// My groupmate tried bash time with an analogue clock. It turned out, that to get a real time, you gotta multiply by 10000...			
				} else {
					
	// All the other commands, which may contain '>' or '<'
			
					int n_words = commands[0].size();
					string output_file_name;
					string input_file_name;
					int flag_more = 0;
					int flag_less = 0;
                    int more_count = 0;
                    int less_count = 0;
                    int two_or_more_same_signs = 0;
                    if ((commands[0][0] == ">") || (commands[0][0] == "<")) {
						printf("A command can't start with a < or > sign\n");
						continue;
					}
                    if (((commands[0][n_words-1] == ">") || (commands[0][n_words-1] == "<")) && (commands[0][0] != "cat")) {
						printf("A command can't finish with a < or > sign\n");
						continue;
					}
                    if ((commands[0][n_words-1] == ">") && (commands[0][0] == "cat")) {
						printf("A cat command can't finish with a > sign\n");
						continue;
					}
					if ((n_words != 2) && (commands[0][0] == "cat") && (commands[0][n_words-1] == "<"))	{
						printf("Please check that command is right\n");
						continue;
					}														
					for (int i = 0; i < n_words - 1; i++) {
						if (commands[0][i] == ">") {
							output_file_name = commands[0][i+1];
							flag_more = 1;
                            more_count += 1;
							if ((output_file_name == ">") or (output_file_name == "<")) {
								perror("A filename is missed :(\n");
							}
						}
						if (commands[0][i] == "<") {
							input_file_name = commands[0][i+1];
							flag_less = 1;
                            less_count += 1;
							if ((input_file_name == ">") or (input_file_name == "<")) {
								perror("A filename is missed :(\n");
							}				
						}
                        if ((more_count > 1) | (less_count > 1)) {
                            printf("The > symbol can't be mentioned more than once\n in a command line (the same for <)\n");
                            two_or_more_same_signs = 1;
                        }                        
					}
					if (two_or_more_same_signs) {
						continue;
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
						signal(2, SIG_DFL);					
						if (flag_less) {
							int in_fd = open((const char*) input_file_name.c_str(), O_RDONLY | O_CREAT, 0666);
							dup2(in_fd, 0);
						}
						if (flag_more) {	
							int out_fd = open((const char*) output_file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
							dup2(out_fd, 1);
						}
						int status = execvp(command[0], (char * const*)&command[0]);
						if (status != 0) printf("Please make sure the name of your command is correct\n");
						exit(status);
					} else {
						int code;
						wait(&code);
					}
				}
			} else if (commands.size() > 1) {			
	// CASE 2. PIPES
				if (!check_f(commands)) {
					pid_t pid = fork();
					
					if (pid == 0) {
						signal(2, SIG_DFL);						
						peter_piper(commands);			
					} else {
						int code;
						wait(&code);
					}																
				} else {
					printf("Please make sure that only the first command\nin your pipeline contains an input redirection\nand only the last one output redirection.\nAlso make sure you did not miss a file name.\n");
					continue;
				}					
			}				
		} else {
			continue;	
		}
	}
	return 0;	
}

