<h1>smallsh</h1>
smallsh is a small shell program written in the C language. This program was my portfolio project for my Operating Systems class at Oregon State University.

<h2>About</h2>
smallsh implement's a subset of features of well-known shells, such as bash.
Executes 3 commands exit, cd, and status via code built into the shell.
Executes other commands by creating new processes using execvp.
Supports input and output redirection, and running commands in foreground and background processes.
Uses custom handlers for SIGINT and SIGTSTP.
Any instance of '$$' is expanded into the process id of smallsh.
<h2>Compilation</h2>
This program was tested using the GNU99 standard. To compile use the following command<br>
<code>gcc -std=gun99 -o smallsh smallsh.c</code>
