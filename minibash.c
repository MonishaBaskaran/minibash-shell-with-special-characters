#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>

#define MAX_PARAM 5
#define MAX_PIPES 4
#define MAX_COND 4

// Array to store PIDs of background processes
int bg_processes[10] = {0};
int bg_count = 0;

// Function prototypes
void execute_command(char *cmd, int background);
void handle_special_characters(char *cmd);
void count_words(char *filename);
void cat_files(char *cmd);
void piping(char *cmd);
void sequential(char *cmd);
void redirection(char *cmd);
void conditional(char *cmd);
char **split_string(char *str, const char *delim);
void bg_to_fore();
int check_argument_count(char **args);

// Function to count words in a file
void count_words(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        // Print error if file cannot be opened
        fprintf(stderr, "Error: fopen: No such file or directory: %s\n", filename);
        return;
    }

    int word_count = 0;
    char word[1024];
    // Read words from file and count them
    while (fscanf(file, "%1023s", word) == 1) {
        word_count++;
    }

    printf("Word count: %d\n", word_count);
    fclose(file);
}

// Function to concatenate multiple files and print their contents
void cat_files(char *cmd) {
    char *token = strtok(cmd, "~");
    int op_count = 0;
    while (token != NULL) {
        // Trim leading spaces
        while (isspace((unsigned char)*token)) token++;
        
        // Trim spaces and newlines
        char *filename = token;
        size_t len = strlen(filename);
        while (len > 0 && (isspace((unsigned char)filename[len - 1]) || filename[len - 1] == '\n')) {
            filename[--len] = '\0';
        }

        FILE *file = fopen(filename, "r");
        if (file == NULL) {
            // Print error if file cannot be opened
            fprintf(stderr, "Error: fopen: No such file or directory: %s\n", filename);
            return;
        }

        char line[1024];
        // Read lines from file and print them
        while (fgets(line, sizeof(line), file) != NULL) {
            printf("%s", line);
        }

        fclose(file);
        token = strtok(NULL, "~");
        op_count++;
        // Check for too many concatenation operations
        if (op_count > 4) {
            fprintf(stderr, "Error: Too many concatenation operations.\n");
            return;
        }
    }
}

// Function to execute a single command
void execute_command(char *cmd, int background) {
    char *args[MAX_PARAM] = {NULL};
    char *token = strtok(cmd, " ");
    int i = 0;

    // Tokenize the command into arguments
    while (token != NULL && i < MAX_PARAM - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    // Check for valid number of arguments
    if (i < 1 || i > 4) {
        fprintf(stderr, "Error: Invalid number of arguments for command.\n");
        return;
    }

    // Handle "dter" command to kill minibash terminal
    if (strcmp(args[0], "dter") == 0) {
        exit(0);
    }

    pid_t pid = fork();

    if (pid == 0) { // Child process
        // Execute the command
        if (execvp(args[0], args) == -1) {
            perror("minibash");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) { // Forking error
        perror("minibash");
    } else { // Parent process
        if (background) {
            printf("Background process started with PID: %d\n", pid);
            if (bg_count < 10) {
                bg_processes[bg_count++] = pid;
            } else {
                fprintf(stderr, "Error: Maximum background processes reached.\n");
            }
        } else {
            int status;
            // Wait for the child process to finish
            waitpid(pid, &status, 0);
        }
    }
}

// Function to handle command piping
void piping(char *cmd) {
    int fd[2];
    pid_t pid;
    int fd_in = 0;
    int i = 0;

    char **commands = split_string(cmd, "|");

    // Execute each command in the pipeline
    while (commands[i] != NULL && i < MAX_PIPES) {
        pipe(fd);
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // Redirect input and output
            dup2(fd_in, 0);
            if (commands[i + 1] != NULL) {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            char **args = split_string(commands[i], " ");
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        } else {
            waitpid(pid, NULL, 0);
            close(fd[1]);
            fd_in = fd[0];
            i++;
        }
    }

    // Free allocated memory for commands
    i = 0;
    while (commands[i] != NULL) {
        free(commands[i]);
        i++;
    }
    free(commands);
}

// Function to handle sequential commands
void sequential(char *cmd) {
    char *commands[MAX_PIPES + 1] = {NULL}; // Ensure space for MAX_PIPES + 1 commands
    int i = 0;

    char *token = strtok(cmd, ";");
    while (token != NULL && i < MAX_PIPES) {
        commands[i++] = token;
        token = strtok(NULL, ";");
    }
    commands[i] = NULL;

    for (int j = 0; j < i; j++) {
        while (isspace((unsigned char)*commands[j])) commands[j]++; // trim leading spaces
        handle_special_characters(commands[j]);
    }
}

// Function to handle input and output redirection
void redirection(char *cmd) {
    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;

    // Check for input redirection
    char *input_redirect = strchr(cmd, '<');
    if (input_redirect != NULL) {
        *input_redirect = '\0';
        input_file = strtok(input_redirect + 1, " ");
    }

    // Check for output redirection
    char *output_redirect = strchr(cmd, '>');
    if (output_redirect != NULL) {
        if (*(output_redirect + 1) == '>') {
            append = 1;
            *output_redirect = '\0';
            output_file = strtok(output_redirect + 2, " ");
        } else {
            *output_redirect = '\0';
            output_file = strtok(output_redirect + 1, " ");
        }
    }

    while (isspace((unsigned char)*cmd)) cmd++; // trim leading spaces

    pid_t pid = fork();

    if (pid == 0) { // Child process
        if (input_file != NULL) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1) {
                perror("minibash");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (output_file != NULL) {
            int fd_out;
            if (append) {
                fd_out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } else {
                fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            if (fd_out == -1) {
                perror("minibash");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        char **args = split_string(cmd, " ");
        if (!check_argument_count(args)) {
            fprintf(stderr, "Error: Invalid number of arguments for command.\n");
            exit(1);
        }
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid < 0) { // Forking error
        perror("minibash");
    } else { // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

// Function to handle conditional execution of commands
void conditional(char *cmd) {
    char *commands[MAX_COND + 1] = {NULL};
    int cmd_status[MAX_COND + 1] = {0};
    char *operators[MAX_COND] = {NULL};
    int i = 0, j = 0;
    char *ptr = cmd;

    // Split commands and operators
    commands[i++] = ptr;
    while (*ptr) {
        if (*ptr == '&' && *(ptr + 1) == '&') {
            operators[j++] = "&&";
            *ptr = '\0';
            ptr += 2;
            commands[i++] = ptr;
        } else if (*ptr == '|' && *(ptr + 1) == '|') {
            operators[j++] = "||";
            *ptr = '\0';
            ptr += 2;
            commands[i++] = ptr;
        } else {
            ptr++;
        }
    }

    // Execute commands based on conditions
    for (int k = 0; k < i; k++) {
        char **args = split_string(commands[k], " ");
        if (!check_argument_count(args)) {
            fprintf(stderr, "Error: Invalid number of arguments for command in conditional execution.\n");
            return;
        }

        pid_t pid = fork();
        if (pid == 0) { // Child process
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid < 0) { // Forking error
            perror("fork");
            exit(1);
        } else { // Parent process
            int status;
            waitpid(pid, &status, 0);
            cmd_status[k] = (status == 0) ? 1 : 0;

            // Check the operator before the next command
            if (k < j) {
                if ((strcmp(operators[k], "&&") == 0 && !cmd_status[k]) || 
                    (strcmp(operators[k], "||") == 0 && cmd_status[k])) {
                    break;
                }
            }
        }
    }
}

// Function to handle special characters in the command
void handle_special_characters(char *cmd) {
    if (strchr(cmd, ';') != NULL) {
        sequential(cmd);
    } else if (strchr(cmd, '<') != NULL || strchr(cmd, '>') != NULL) {
        redirection(cmd);
    } else if (cmd[0] == '#') {
        char *filename = cmd + 1;
        while (isspace((unsigned char)*filename)) filename++; // trim leading spaces
        count_words(filename);
    } else if (strchr(cmd, '~') != NULL) {
        cat_files(cmd);
    } else if (strstr(cmd, "&&") != NULL || strstr(cmd, "||") != NULL) {
        conditional(cmd);
    } else if (strchr(cmd, '|') != NULL) {
        piping(cmd);
    } else if (strstr(cmd, "fore") != NULL) {
        bg_to_fore();
    } else if (strstr(cmd, "dter") != NULL) {
        printf("Exiting minibash terminal...\n");
        exit(0);
    } else {
        int background = (cmd[strlen(cmd) - 1] == '+');
        if (background) {
            cmd[strlen(cmd) - 1] = '\0'; // Remove the '+' sign
        }
        execute_command(cmd, background);
    }
}

// Function to split a string by a delimiter
char **split_string(char *str, const char *delim) {
    char **result = malloc(MAX_PARAM * sizeof(char *));
    char *token;
    int i = 0;

    token = strtok(str, delim);
    while (token != NULL && i < MAX_PARAM - 1) {
        result[i] = strdup(token);
        token = strtok(NULL, delim);
        i++;
    }
    result[i] = NULL;
    return result;
}

// Function to bring the last background process to the foreground
void bg_to_fore() {
    if (bg_count == 0) {
        printf("No background process to bring to foreground.\n");
        return;
    }

    pid_t pid = bg_processes[--bg_count];
    int status;
    waitpid(pid, &status, 0);
    printf("Background process %d brought to foreground.\n", pid);
}

// Function to check the number of arguments
int check_argument_count(char **args) {
    int count = 0;
    while (args[count] != NULL) {
        count++;
    }
    return count >= 1 && count <= 4;
}

int main() {
    char cmd[1024];

    while (1) {
        printf("minibash$ ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        
        // Remove trailing newline character
        size_t len = strlen(cmd);
        if (len > 0 && cmd[len - 1] == '\n') {
            cmd[len - 1] = '\0';
        }

        // Handle special characters in the command
        handle_special_characters(cmd);
    }

    return 0;
}
