# minibash-shell-with-special-characters

Write a C program minibash (minibash$) that goes into an infinite loop waiting for user's 
commands. Once a command is entered, the program should assemble and execute each 
command using fork(), exec() and other system calls as required with the following rules 
and conditions. 

NOTE: You cannot use the system() library function 
A modular approach would be helpful. 

Rule 1: 
 The command dter ( minibash$dter ) must kill the current minibash terminal 

Rule 2: //Excluded from the requirements 

Rule 3: The argc (includes the name of the executable/command) of any command/ 
program should be >=1 and < =4 

Examples: 
 minibash$ pwd (argc =1)

 minibash$ grep to sample.txt (argc=3)

 minibash$ ls -l -t -a (argc =4)

Special Characters 
The program should handle the following special characters. 

 # Counts the number of words present in a particular .txt file
minibash$ # sample.txt //should output the number of words in 
sample.txt

 ~ Text file (.txt) concatenation (up to 4 operations)
 Ex minibash$ sample1.txt ~ sample2.txt ~ temp1.txt ~ temp2.txt 
 // Files must be concatenated in the order in which they are listed and 
the final result is displayed on stdout 

 + Creates a running process in the background
 minibash$ ex10 30 20 a + //should run “ex10 30 20 a” in the 
background
 the argc of the command/program should be >=1 and <=4
 //Note: minibash$fore should bring the last process pushed to the 
background into the foreground of the minibash on which fore is 
executed . No need to implement CTR-Z

 | Piping (up to 4 piping operations should be supported)
 Ex minibash$ ls -l -t -a |grep *.txt|wc| wc -w 
 // Each command/program should have argc >=1 and <=4 
 
 <,>, >> Input Redirection from a text file, Output Redirection to a text file, Output 
redirection to a text file (with append output) 
 minibash$ grep to <sample.txt 
 minibash$ ls -1 >dirlist.txt 
minibash$ ls -1 >>dirlist.txt 
 The argc of each command/program should be >=1 and <=4 

 ; Sequential execution of commands (up to 4 commands). The argc of 
 each command should be >=1 and <=4 
 Ex: minibash$ date ;pwd ;ls -l – t -a 
 
 && Conditional Execution // up to 4 conditional execution operators should 
 be supported and could possibly be a combination of && and || 
 Ex : minibash$ ex1 && ex2 && ex3 && ex4 
 minibash$ c1 && c2 || c3 && c4 

 || Conditional Execution // see && 
o Note in both && and ||, the argc of each command should be >=1 and 
<=4 

You are NOT required to combine special characters : ex $ p1 & p2 > list.txt 
Note: 
 You must include comments throughout the program reasonably explaining the 
working of the code. 
 You have to use fork() and exec() along with other pertinent system calls to run 
commands from minishell //You cannot use the system() function. 
 Appropriate error messages must be displayed by the program based on the 
specifications. 
