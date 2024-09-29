# ish: A Custom Unix Shell
## Overview
The ish project is a custom Unix shell written in C with functionality inspired by the csh shell. 

This shell supports job control, I/O redirection, and built-in commands, enabling users to execute simple and complex command sequences with features like pipelines, environment variable management, and background processing.

## Features
Simple command execution: Supports typical Unix commands like ls, cd, pwd, and others.

Job control: Run commands in the background with &, bring jobs to the foreground, and manage running jobs with jobs, bg, and fg.

I/O Redirection: Redirect input and output using <, >, >>, and their variants for both standard error and standard output.

Pipelines: Chain commands using | and |& for redirecting standard output and error streams.

Environment Variables: Set and unset environment variables with setenv and unsetenv, and inherit the shell's PATH variable for command lookup.

## Built-in Commands:

cd [directory]: Change the working directory.

exit: Exit the shell.

jobs: List active jobs.

fg %job: Bring a background job to the foreground.

bg %job: Continue a stopped job in the background.

setenv [VAR] [value]: Set environment variables.

unsetenv [VAR]: Remove an environment variable.


## Usage
Once the shell is started, it displays a prompt in the format hostname%, where you can enter commands. The shell supports the following key functionalities:

Running commands:

ls -l

I/O Redirection:

ls > output.txt    # Redirect output to a file
cat < input.txt    # Use a file as input

Pipelines:

ls | grep ".txt"   # Use output from one command as input for another

Background Processes:

sleep 10 &         # Run command in the background

### Compile the source code:

make

### Run the shell:

./ish
