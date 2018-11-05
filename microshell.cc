#include <stdio.h>
#include <stdlib.h>

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

using namespace std;

string HOME_DIR;
string CURR_DIR;

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

void run_command(string &path, vector<string> &v) {
	pid_t pid = fork();

	vector<const char *> argv;
	for (vector<string>::size_type i = 0; i != v.size(); ++i) {
		const char *tmp = v[i].c_str();
		argv.push_back(tmp);
	}
	argv.push_back(NULL);
      
    if (pid == 0) {
        execvp(argv[0], (char * const*)&argv[0]);
		perror("run_command");
    } else {
        int code;
        wait(&code);
   }
}

int main(int argc, char **argv, char **envp) {
	// getting shell's home directory
	char buffer[100];
	strcpy(buffer, &envp[23][5]);
	HOME_DIR = (string) buffer;
	
	// here is the programme per se
	while (1) {
	// let's greet the user and get his command line
		string s;
		greeting_f(&s);
	// let's parse this line into commands
		vector<string> v = split(s);
	// Commands cd and pwd are very special. In the other cases we will use run_command function.	
		if (v[0] == "cd") {
			if (v.size() == 1) {
				chdir(HOME_DIR.c_str());
			} else {
				if (chdir(v[1].c_str()) < 0) {
					perror("PATH DOES NOT EXIST!");
				}
			}
		} else if (v[0] == "pwd") {
			printf("%s", CURR_DIR.c_str());
		} else {
			string path = "/bin/" + v[0];
			run_command(path, v);
		}
	}
	return 0;
}
