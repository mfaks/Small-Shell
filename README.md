# Small Shell - Custom Shell Implementation in C

## Introduction
This project implements a custom shell called smallsh in C. The smallsh shell provides a subset of features similar to well-known shells like bash. It allows users to run commands, handle blank lines and comments, expand variables, execute built-in commands, execute other commands via processes, support input/output redirection, run commands in foreground and background processes, and implement custom signal handlers.

## Description
smallsh is a custom shell designed to provide basic shell functionalities such as executing commands, managing processes, handling signals, and supporting input/output redirection. It is developed in C and aims to replicate key features of popular Unix shells like bash.

## Key Components
1. **Command Prompt**: Utilizes the colon `:` symbol as a prompt for each command line. Supports command syntax with optional input/output redirection and background execution.
2. **Built-in Commands**: Supports three built-in commands: `exit`, `cd`, and `status`. Handles execution of built-in commands separately from external commands.
3. **Execution of Other Commands**: Executes non-built-in commands using fork(), exec(), and waitpid(). Utilizes the PATH variable to locate non-built-in commands.
4. **Input & Output Redirection**: Redirects input/output using dup2() before executing the command. Handles errors if files cannot be opened for input/output redirection.
5. **Foreground & Background Execution**: Runs commands in foreground or background based on the presence of `&`. Prints process IDs and exit statuses for background processes.
6. **Signal Handling**: Handles SIGINT and SIGTSTP signals appropriately for foreground and background processes. Displays informative messages and manages shell behavior based on signal reception.

## Compilation Instructions
To compile the smallsh shell, follow these steps:
1. Open a terminal.
2. Navigate to the directory containing the source code files.
3. Use the gcc compiler to compile the code:    
   `gcc -o smallsh small_shell.c`
5. Upon successful compilation, an executable file named `smallsh` will be created in the same directory.
6. Download the testing script provided in this repository and run the command:     
    `./smallsh`
