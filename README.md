smallsh is a shell written in C. smallsh implement's a subset of features of well-known shells, such as bash.
Executes 3 commands exit, cd, and status via code built into the shell.
Executes other commands by creating new processes using execvp.
Supports input and output redirection, and running commands in foreground and background processes.
Uses custom handlers for SIGINT and SIGTSTP.
Any instance of '$$' is expanded into the process id of smallsh.
