#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>


#define MAX_COMMAND_LEN 511
//A struct of variables
typedef struct {
    char name[MAX_COMMAND_LEN];
    char value[MAX_COMMAND_LEN];
    struct variable *next;
} variable;

char *inputCommand(char temp[]);

char *my_strtok(char *str, const char *delim);

void freeVariable(variable *head_vars);

void delete_char_at_index(char *str, int index);

variable *create_var(char **div_var);

char *findVar(char *command, variable **head_vars);

void saveVar(char *command, variable **head_vars);

int findIndex(char target_char, char *str);

void catchChild(int sig);

void insert_char_at_index(char *str, int index, char ch);

void catch_ctrlZ(int sig);

pid_t pid = -1;
int pidStp;

int main() {
    // signal processing
    signal(SIGTSTP, catch_ctrlZ);
    signal(SIGCHLD, catchChild);
    int numCommands = 0, numArgs = 0, numEnter = 0;
    char command[MAX_COMMAND_LEN], cwd[MAX_COMMAND_LEN];
    //The main command set
    char *commands[MAX_COMMAND_LEN];
    variable *head_vars = NULL;
    // A pipe for a write command to a file
    int pipe_for_redi[2];

    // find the current working dir
    getcwd(cwd, sizeof(cwd));

    // the loop of the prompt
    while (1) {
        printf("#cmd:%d|#args:%d@%s>", numCommands, numArgs, cwd);
        char *temp = inputCommand(command); //A function that receives input from the user.
        if (command[0] == 0) {  // Validity check for input
            printf("ERR\n");
            break;
        }
        //Division into an array of commands
        int i = 0;
        char *command_current = my_strtok(command, ";");
        while (command_current != NULL) {
            commands[i++] = command_current;
            command_current = my_strtok(NULL, ";");
        }
        commands[i] = NULL;
        //Test for pressing enter
        if (commands[0][0] == '\n') {
            numEnter++;
            if (numEnter > 2) {
                freeVariable(head_vars);
                exit(0);
            }
        } else {
            //The main command loop
            for (int j = 0; commands[j] != NULL; ++j) {
                numEnter = 0;
                commands[j][strcspn(commands[j], "\n")] = '\0';           // Deleting enter from the end of the command
                if (strcmp(commands[j], "cd") == 0) {                         //Testing with the command is "cd"
                    printf("cd not supported\n");
                    continue;
                }
                bool hasPipe = false;  // for |
                bool hasBg = false;  // for bg
                bool hasAmper = false; // for &
                bool hasRead = false; // for >
                char temp2[MAX_COMMAND_LEN];
                strcpy(temp2, commands[j]);
                int num_comm_pipe = 0;
                i = 0;
                int checkDolar = (int) strspn("$", temp2); //find if the char | in the command
                int checkPipe = (int) strspn("|", temp2); // find if the char | in the command
                int checkAmper = (int) strspn("&", temp2); // find if the char & in the command
                int checkEqual = (int) strspn("=", temp2); // find if the char = in the command
                int checkRead = (int) strspn(">", temp2); // find if the char > in the command
                char *args[11] = {NULL};
                //Creating a pipe of the father
                int pipeFd1[2];
                if (pipe(pipeFd1) == -1) {
                    perror("ERR pipe");
                    exit(1);
                }
                char *arg;
                char *command_use_pipe[MAX_COMMAND_LEN];
                char *commandRead[MAX_COMMAND_LEN];
                char *replace_dolar;
                char *value_var;
                char name_var[MAX_COMMAND_LEN];
                char *currentCom;
                if (checkDolar) {
                    char *tempDolar;
              //Checking if there are variables in the command and if they should be replaced with their value
                 while (checkDolar != 0) {
                        int k, r = 0;
                        replace_dolar = strchr(temp2, '$');
                        for (k = 0; k < strlen(replace_dolar); ++k) {
                            if ((replace_dolar[k] != '"') && replace_dolar[k] != ' ')
                                name_var[k] = replace_dolar[k];
                        }
                        name_var[k] = 0;
                        value_var = findVar(name_var, &head_vars);
                        if (value_var != NULL) {
                            int x = findIndex('$', temp2);
                            if (strlen(name_var) >= strlen(value_var)) {
                                int rmD = 0;
                                while (strlen(name_var) > r) {
                                    if (r < strlen(value_var)) {
                                        temp2[x + r] = value_var[r];
                                    } else {
                                        delete_char_at_index(temp2, x + r - rmD);
                                        rmD++;
                                    }
                                    r++;
                                }
                            } else {
                                while (strlen(value_var) >= r) {
                                    if (r < strlen(name_var)) {
                                        temp2[x + r] = value_var[r];
                                    } else {
                                        insert_char_at_index(temp2, x + r, value_var[r]);
                                    }
                                    r++;
                                }
                            }
                            checkDolar = (int) strspn("$", temp2);
                        } else {
                            break;
                        }
                    }
                }
                //Checking if there is a command that uses a pipe or a command that writes to a file
                if (checkPipe || checkRead) {
                    if (checkRead) {
                        hasRead = true;
                        currentCom = strtok(temp2, ">");
                    } else {
                        hasPipe = true;
                        currentCom = strtok(temp2, "|");
                    }
                    while (currentCom != NULL) {
                        command_use_pipe[num_comm_pipe++] = currentCom;
                        if (hasPipe) { currentCom = strtok(NULL, "|"); } else { currentCom = strtok(NULL, ">"); }
                    }
                    command_use_pipe[num_comm_pipe] = NULL;
                }
                if (checkEqual == 0) {
                    // check & in command
                    if (checkAmper) {
                        hasAmper = true;
                        temp2[strcspn(commands[j], "&")] = '\0';
                    }
                    if (strcmp(temp2, "bg") == 0) {
                        kill(pidStp, SIGCONT);// check why the func kill the process
                        break;
                    }

                    if (num_comm_pipe == 0) { num_comm_pipe = 1; }

                    //loop of the commands div with " "
                    for (int k = 0; k < num_comm_pipe; ++k) {
                        int pipeFd2[2], argsInCommand = 0;
                        if (k < num_comm_pipe - 1 && pipe(pipeFd2) == -1) {
                            perror("ERR");
                            exit(EXIT_FAILURE);
                        }
                        if (hasPipe || hasRead) {
                            arg = my_strtok(command_use_pipe[k], " ");
                        } else { arg = my_strtok(temp2, " "); }
                        while (arg != NULL) {
                            checkDolar = (int) strspn("$", arg); // find if the char $ in the command
                            if (checkDolar) {
                                // There is the character $
                                args[i++] = findVar(arg, &head_vars);
                                arg = strtok(NULL, " ");
                                continue;
                            }
                            args[i++] = arg;
                            argsInCommand++;
                            arg = my_strtok(NULL, " ");
                            if (i > 10) {
                                break;
                            }
                        }
                        if (i > 10)
                            break;
                        args[i] = NULL;
//Checking if there are quotation marks in the command, if so remove them
                        i = 0;
                        int checkQ = 0;
                        for (int l = 0; l < argsInCommand; ++l) {
                            checkQ = (int) strspn("\"", args[l]);
                            if (checkQ) {
                                char removeQ[MAX_COMMAND_LEN]= {0};
                                for (int m = 0; m < strlen(args[l]); ++m) {
                                    if (args[l][m] != '"')
                                        removeQ[i++] = args[l][m];
                                }
                                strcpy(args[l], removeQ);
                            }
                        }

                        // Creating a process
                        pid = fork();
                        if (pid < 0) {
                            perror("ERR");
                            freeVariable(head_vars);
                            exit(1);
                        }
                        // the son process
                        if (pid == 0) {
                            //open file
                            if (hasRead) {
                                int fileFd;
                                char *name_file;
                                name_file = strcpy(cwd, "/");
                                strcpy(name_file, command_use_pipe[k + 1]);
                                if ((fileFd = open(name_file, O_CREAT | O_RDONLY | O_WRONLY, 0777)) == -1) {
                                    perror("ERR");
                                    freeVariable(head_vars);
                                    exit(EXIT_FAILURE);
                                }
                                dup2(fileFd, STDOUT_FILENO);

                            }
                            //open pipe
                            if (hasPipe) {
                                if (k > 0) {
                                    // Connect stdin to previous command's pipe
                                    if (dup2(pipeFd1[0], STDIN_FILENO) == -1) {
                                        perror("ERR dup");
                                        freeVariable(head_vars);
                                        exit(EXIT_FAILURE);
                                    }
                                    close(pipeFd1[0]);
                                    close(pipeFd1[1]);
                                }
                                //the first command
                                if (k < num_comm_pipe - 1) {
                                    // Connect stdout to next command's pipe
                                    if (dup2(pipeFd2[1], STDOUT_FILENO) == -1) {
                                        perror("ERR dup");
                                        freeVariable(head_vars);
                                        exit(EXIT_FAILURE);
                                    }
                                    close(pipeFd2[0]);
                                    close(pipeFd2[1]);
                                }
                            }
                            execvp(args[0], args);
                            perror("ERR");
                            freeVariable(head_vars);
                            exit(1);
                        } // the father process
                        else {
                            if (!hasAmper) {
                                if (hasPipe) {
                                    if (k > 0) {
                                        close(pipeFd1[0]);
                                        close(pipeFd1[1]);
                                    }
                                    pipeFd1[0] = pipeFd2[0];
                                    pipeFd1[1] = pipeFd2[1];
                                }
                                int status;
                                waitpid(pid, &status, WUNTRACED);
                                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                                    numCommands++;
                                    numArgs += argsInCommand;
                                }
                            }
                        }
                    }
                } else {
                    saveVar(commands[j], &head_vars);
                }
            }
        }
    }

    freeVariable(head_vars);
    return 0;
}
//A function that inserts a character into a certain index.
void insert_char_at_index(char *str, int index, char ch) {
    int len = (int)strlen(str);

    if (index >= 0 && index <= len) {
        // Shift characters from index to end one position to the right
        for (int i = len; i > index; i--) {
            str[i] = str[i - 1];
        }
        str[index] = ch; // Insert new character at index
    }
}
//A function that finds the index of the first specific character in a string.
int findIndex(char target_char, char *str) {
    int index = -1;
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == target_char) {
            index = i;
            break;
        }
        i++;
    }
    return index;
}


//A function that is done when a ctrl z signal enters
void catch_ctrlZ(int sig) {
    if (pid != 0) {
        pidStp = pid;
        kill(pid, SIGTSTP);
    }
}
//This function is invoked when the status of the child process changes.
void catchChild(int sig) {
    int status;
   waitpid(-1, &status, WNOHANG);
}
//A function that receives input from the user.
char *inputCommand(char temp[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread = getline(&line, &len, stdin);
    if (nread == -1) {
        perror("ERR");
        exit(EXIT_FAILURE);
    }
    if (strlen(line) > MAX_COMMAND_LEN - 1) {
        free(line);
        return NULL;
    }
    strcpy(temp, line);
    free(line);
    return temp;
}

// a function that free all the maloc
void freeVariable(variable *head_vars) {
    while (head_vars != NULL) {
        variable *tem = head_vars;
        head_vars = (variable *) head_vars->next;
        free(tem);
    }
}
//A function that works like strtok but with slight changes that consider the edge case
char *my_strtok(char *str, const char *delim) {
    static char *last = NULL;
    char *token;

    if (str != NULL) {
        last = str;
    } else {
        if (last == NULL) {
            return NULL;
        }
        str = last;
    }

    // Skip any leading delimiters.
    int inside_quote = 0;
    str += strspn(str, delim);
    if (*str == '\0') {
        last = NULL;
        return NULL;
    }

    // Find the end of the token, ignoring delimiters inside quotes.
    token = str;
    while (*str != '\0') {
        if (*str == '"') {
            inside_quote = !inside_quote;
        } else if (!inside_quote && strchr(delim, *str)) {
            break;
        }
        str++;
    }
    if (*str == '\0') {
        last = NULL;
    } else {
        *str = '\0';
        last = str + 1;
    }

    return token;
}
// A function that deletes a character from a string at a certain index.
void delete_char_at_index(char *str, int index) {
    int len = (int)strlen(str);

    if (index >= 0 && index < len) {
        // Shift characters from index+1 to end one position to the left
        for (int i = index; i < len - 1; i++) {
            str[i] = str[i + 1];
        }
        str[len - 1] = '\0'; // Terminate string after last character
    }
}

//A function that saves the variable into an element in the linked list
void saveVar(char *command, variable **head_vars) {
// Decomposition of the command into an array
    char *div_var[2] = {NULL};
    int i = 0;
    char *arg = strtok(command, "=");
    while (arg != NULL) {
        div_var[i++] = arg;
        arg = strtok(NULL, "=");
    }
    if (i != 2) {
        printf("Invalid variable declaration\n");
        return;
    }
    //Dynamic allocation
    variable *new_var = create_var(div_var);
    if (*head_vars == NULL) {
        *head_vars = new_var;
    } else {
        variable *temp = *head_vars;
        while (temp->next != NULL) {
            temp = (variable *) temp->next;
        }
        temp->next = (struct variable *) new_var;
    }
}

//Initialize a new variable
variable *create_var(char **div_var) {
    variable *current = (variable *) malloc(sizeof(variable));
    if (current == NULL) {
        printf("ERR\n");
        exit(1);
    }
    strcpy(current->name, div_var[0]);
    strcpy(current->value, div_var[1]);
    current->next = NULL;
    return current;
}

//A function to find the value of a variable
char *findVar(char *command, variable **head_vars) {
    command = strtok(command, "$");
    if (*head_vars == NULL) {
        return NULL;
    } else {
        variable *current = *head_vars;
        while (current->next != NULL && strcmp(current->name, command) != 0) {
            current = (variable *) current->next;
        }
        if (strcmp(current->name, command) == 0)
            return current->value;
    }
    return NULL;
}

