#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80  /* The maximum length command */
char history[MAX_LINE];

int main(void)
{
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */
    char input[MAX_LINE];

    while (should_run) {
        printf("osh> ");
        fflush(stdout);
        if (fgets(input, MAX_LINE, stdin) == NULL){
            perror("fgets failed");
            return 1;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "!!") == 0){
            if (strlen(history) == 0){
                printf("No commands in history.\n");
                continue;
            } else {
                strcpy(input, history);
                printf("%s\n", input);
            }
        } else {
            strcpy(history, input);
        }

        // Check for pipe
        char *pipe_pos = strchr(input, '|');
        if (pipe_pos != NULL) {
            *pipe_pos = '\0';
            pipe_pos++;

            char *args1[MAX_LINE/2 + 1];
            char *args2[MAX_LINE/2 + 1];

            // Tokenize the first part of the command
            int i = 0;
            char *token = strtok(input, " ");
            while (token != NULL) {
                args1[i++] = token;
                token = strtok(NULL, " ");
            }
            args1[i] = NULL;

            // Tokenize the second part of the command
            i = 0;
            token = strtok(pipe_pos, " ");
            while (token != NULL) {
                args2[i++] = token;
                token = strtok(NULL, " ");
            }
            args2[i] = NULL;

            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("pipe failed");
                return 1;
            }

            pid_t pid1 = fork();
            if (pid1 < 0) {
                perror("Fork failed");
                return 1;
            } else if (pid1 == 0) {
                // Child process for first command
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                execvp(args1[0], args1);
                perror("execvp failed");
                return 1;
            }

            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("Fork failed");
                return 1;
            } else if (pid2 == 0) {
                // Child process for second command
                dup2(pipe_fd[0], STDIN_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                execvp(args2[0], args2);
                perror("execvp failed");
                return 1;
            }

            close(pipe_fd[0]);
            close(pipe_fd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        } else {
            int i = 0;
            char *token = strtok(input, " ");
            char *input_file = NULL;
            char *output_file = NULL;
            int background = 0;

            while (token != NULL){
                if (strcmp(token, ">") == 0) {
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        output_file = token;
                    } else {
                        fprintf(stderr, "No output file specified.\n");
                        continue;
                    }
                } else if (strcmp(token, "<") == 0) {
                    token = strtok(NULL, " ");
                    if (token != NULL) {
                        input_file = token;
                    } else {
                        fprintf(stderr, "No input file specified.\n");
                        continue;
                    }
                } else if (strcmp(token, "&") == 0) {
                    background = 1;
                } else {
                    args[i++] = token;
                }
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            if (args[0] == NULL) {
                continue;
            }

            if (strcmp(args[0], "exit") == 0){
                should_run = 0;
                continue;
            }

            pid_t pid = fork();
            if (pid < 0){
                perror("Fork Failed");
                return 1;
            } else if (pid == 0){
                // Handle input redirection
                if (input_file != NULL) {
                    int fd = open(input_file, O_RDONLY);
                    if (fd < 0) {
                        perror("open failed");
                        return 1;
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }

                // Handle output redirection
                if (output_file != NULL) {
                    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                        perror("open failed");
                        return 1;
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                execvp(args[0], args);
                perror("execvp failed");
                return 1;
            } else {
                if (!background) {
                    waitpid(pid, NULL, 0);
                } else {
                    printf("Process %d running in background\n", pid);
                }
            }
        }
    }
    return 0;
}
