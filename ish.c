#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_ARGS 32
#define MAX_LINE 1024
#define MAX_JOBS 32

struct Job {
    pid_t pid;
    int job_num;
    char cmd[MAX_LINE];
    int suspended;
};

struct Job jobs[MAX_JOBS];
int num_jobs = 0;

char hostname[MAX_LINE]; 

void handle_sigint(int signum) {
    (void)signum;
    //printf("\n");
    // printf("%s%% ", hostname); 
    fflush(stdout);
}

void handle_sigtstp(int signum) {
    (void)signum;
    signal(SIGTSTP, handle_sigtstp); // Reinstall the signal handler
    //printf("\n");
    // printf("%s%% ", hostname); 
    fflush(stdout);
}

void execute_cd(char *args[]) {
    if (args[1] == NULL)
        chdir(getenv("HOME"));
    else
        chdir(args[1]);
}

void execute_exit() {
    exit(0);
}

void execute_jobs() {
    printf("Listing active jobs:\n");
    for (int i = 0; i < num_jobs; i++) {
        printf("[%d] %d\n", jobs[i].job_num, jobs[i].pid);
    }
}
void execute_fg(char *args[]) {
    if (args[1] != NULL) {
        int job_num = atoi(args[1] + 1);
        for (int i = 0; i < num_jobs; i++) {
            if (jobs[i].job_num == job_num) {

                waitpid(jobs[i].pid, NULL, WUNTRACED);
                for (int j = i; j < num_jobs - 1; j++) {
                    jobs[j] = jobs[j + 1];
                }
                num_jobs--;
                return;
            }
        }
        printf("Job %d not found.\n", job_num);
    } else {
        printf("Usage: fg %%job\n");
    }
}

void execute_bg(char *args[]) {
    if (args[1] != NULL) {
        int job_num = atoi(args[1] + 1);
        for (int i = 0; i < num_jobs; i++) {
            if (jobs[i].job_num == job_num) {

                kill(jobs[i].pid, SIGCONT);
                return;
            }
        }
        printf("Job %d not found.\n", job_num);
    } else {
        printf("Usage: bg %%job\n");
    }
}
void execute_kill(char *args[]) {
    if (args[1] != NULL) {
        int job_num = atoi(args[1] + 1);
        for (int i = 0; i < num_jobs; i++) {
            if (jobs[i].job_num == job_num) {
                kill(jobs[i].pid, SIGTERM);
                for (int j = i; j < num_jobs - 1; j++) {
                    jobs[j] = jobs[j + 1];
                }
                num_jobs--;
                return;
            }
        }
        printf("Job %d not found.\n", job_num);
    } else {
        printf("Usage: kill %%job\n");
    }
}

void execute_setenv(char *args[]) {
    if (args[1] == NULL) {
        extern char **environ;
        for (char **env = environ; *env != NULL; env++) {
            printf("%s\n", *env);
        }
    } else if (args[2] == NULL) {
        setenv(args[1], "", 1);
    } else {
        setenv(args[1], args[2], 1);
    }
}

void execute_unsetenv(char *args[]) {
    if (args[1] != NULL) {
        unsetenv(args[1]);
    }
}

void redirect_in(char *fileName) {
    int in = open(fileName, O_RDONLY);
    dup2(in, 0);
    close(in);
}

void redirect_out(char *fileName, int flag) {
    if (flag == 1 || flag == 0) {
        int out = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0600);
        dup2(out, 1);
        dup2(out, 2);
        close(out);
    } else if (flag == 2) {
        int append = open(fileName, O_CREAT | O_RDWR | O_APPEND, 0644);
        dup2(append, STDOUT_FILENO);
        dup2(append, STDERR_FILENO);
        close(append);
    }
}

void execute_command(char *args[], int background) {
    if (strcmp(args[0], "cd") == 0) {
        execute_cd(args);
    } else if (strcmp(args[0], "exit") == 0) {
        execute_exit();
    } else if (strcmp(args[0], "jobs") == 0) {
        execute_jobs();
    } else if (strcmp(args[0], "fg") == 0) {
        execute_fg(args);
    } else if (strcmp(args[0], "bg") == 0) {
        execute_bg(args);
    } else if (strcmp(args[0], "kill") == 0) {
        execute_kill(args);
    } else if (strcmp(args[0], "setenv") == 0) {
        execute_setenv(args);
    } else if (strcmp(args[0], "unsetenv") == 0) {
        execute_unsetenv(args);
    } else {
        pid_t pid;
        int status;
        signal(SIGINT, handle_sigint);
        signal(SIGTSTP, handle_sigtstp);
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
        } else if (pid == 0) {
            dup2(STDOUT_FILENO, STDERR_FILENO);
            execvp(args[0], args);
            fprintf(stderr, "Error: Command '%s' not found\n", args[0]);
            exit(EXIT_FAILURE);
        } else {
            if (!background) {
                waitpid(pid, &status, WUNTRACED);
            } else {
                jobs[num_jobs].pid = pid;
                jobs[num_jobs].job_num = num_jobs + 1;
                strcpy(jobs[num_jobs].cmd, args[0]);
                jobs[num_jobs].suspended = 0;
                num_jobs++;
                printf("[%d] %d %s\n", num_jobs, pid, args[0]);
            }
        }
        redirect_in("/dev/tty");
        redirect_out("/dev/tty", 0);
    }
}

void create_pipe(char *args[], int background) {
    int fd[2];
    pipe(fd);

    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
    } else if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], 1);
        close(fd[1]);
        execvp(args[0], args);
    } else {
        close(fd[1]);
        dup2(fd[0], 0);
        close(fd[0]);
        int status;
        if (!background) {
            waitpid(pid, &status, WUNTRACED);
        } else {
            jobs[num_jobs].pid = pid;
            jobs[num_jobs].job_num = num_jobs + 1;
            strcpy(jobs[num_jobs].cmd, args[0]);
            num_jobs++;
            printf("[%d] %d %s\n", num_jobs, pid, args[0]);
        }
    }
}

char *tokenize(char *input) {
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((MAX_LINE * 2) * sizeof(char));
    int in_redirect = 0;
    for (i = 0; i < strlen(input); i++) {
        if (input[i] != '>' && input[i] != '<' && input[i] != '|' && input[i] != ';' && !(input[i] == '>' && input[i + 1] == '>') && !(input[i] == '>' && input[i + 1] == '&' && input[i + 2] == '>')) {
            tokenized[j++] = input[i];
        } else {
            if (in_redirect) {
                tokenized[j++] = input[i];
            } else {
                if ((input[i] == '>' && input[i + 1] == '>') || (input[i] == '>' && input[i + 1] == '&' && input[i + 2] == '>')) {
                    tokenized[j++] = ' ';
                    tokenized[j++] = input[i++];
                    tokenized[j++] = input[i++];
                    if (input[i] == '&' && input[i + 1] == '>') {
                        tokenized[j++] = input[i++];
                    }
                    tokenized[j++] = ' ';
                } else {
                    tokenized[j++] = ' ';
                    if (input[i] == '>' && input[i + 1] == '&') {
                        tokenized[j++] = input[i++];
                        tokenized[j++] = input[i++];
                    } else {
                        tokenized[j++] = input[i];
                    }
                    if (input[i] == '<' || input[i] == '>') {
                        in_redirect = 1;
                    } else if (input[i] == '&' && input[i + 1] == '>') {
                        tokenized[j++] = input[i++];
                        tokenized[j++] = input[i++];
                        tokenized[j++] = ' ';
                    }
                    tokenized[j++] = ' ';
                }
            }
        }
    }
    tokenized[j++] = '\0';

    char *end;
    end = tokenized + strlen(tokenized) - 1;
    end--;
    *(end + 1) = '\0';

    return tokenized;
}

char *read_input() {
    char *line = NULL;
    size_t bufsize = 0;
    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);  
        } else {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    if (line[0] == '\n') {
        free(line);
        return NULL;
    }
    return line;
}

char **tokenize_commands(char *line) {
    char **commands = malloc(MAX_ARGS * sizeof(char *));
    if (commands == NULL) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    char *token;
    int i = 0;

    token = strtok(line, ";");
    while (token != NULL) {
        commands[i++] = strdup(token);
        token = strtok(NULL, ";");
    }
    commands[i] = NULL;
    return commands;
}

void execute_commands(char **commands, int background) {
    for (int i = 0; commands[i] != NULL; i++) {
        char *tokens = tokenize(commands[i]);
        if (tokens[strlen(tokens) - 1] == '&') {
            background = 1;
            tokens[strlen(tokens) - 1] = '\0';
        } else {
            background = 0;
        }

        char *args[MAX_ARGS];
        char *arg = strtok(tokens, " ");
        int j = 0;
        while (arg) {
            if (*arg == '<') {
                redirect_in(strtok(NULL, " "));
            } else if (*arg == '>') {
                if (*(arg + 1) == '>') {
                    arg = strtok(NULL, " ");
                    redirect_out(arg, 2);
                } else if (*(arg + 1) == '&' && *(arg + 2) == '>') {
                    arg = strtok(NULL, " ");
                    redirect_out(arg, 2);
                } else {
                    redirect_out(strtok(NULL, " "), 1);
                }
            } else if (*arg == '|') {
                args[j] = NULL;
                create_pipe(args, background);
                j = 0;
            } else {
                args[j++] = arg;
            }
            arg = strtok(NULL, " ");
        }
        args[j] = NULL;

        execute_command(args, background);
        free(tokens);
    }
}

void executeCommandsFromFile(const char *filename) {
    FILE *file;
    char command[MAX_LINE];

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error in opening %s file.\n", filename);
        return;
    }

    while (fgets(command, MAX_LINE, file) != NULL) {
        // To remove the trailing space
        if (command[strlen(command) - 1] == '\n') {
            command[strlen(command) - 1] = '\0';
        }

        FILE *cmd_output = popen(command, "r");
        if (cmd_output == NULL) {
            printf("Error executing command: %s\n", command);
        } else {
            char output_line[MAX_LINE];
            while (fgets(output_line, MAX_LINE, cmd_output) != NULL) {
                printf("%s", output_line);
            }
            pclose(cmd_output);
        }
    }
    fclose(file);
}
int main() {
    gethostname(hostname, sizeof(hostname));
 executeCommandsFromFile(".ishrc");
    while (1) {
        printf("%s%% ", hostname);
        fflush(stdout);

        char *line = read_input();
        if (line == NULL) {
            // If no command is entered, prompt again
            continue;
        }

        if (strcmp(line, "exit\n") == 0) {
            free(line);
            break; // Exit the loop and terminate the shell
        }

        char **commands = tokenize_commands(line);
        execute_commands(commands, 1); 
        for (int i = 0; commands[i] != NULL; i++) {
            free(commands[i]);
        }
        free(commands);
        free(line);
    }

    return 0;
}
