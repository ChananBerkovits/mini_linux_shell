# mini_linux_shell
Mini shell
Authored by Chanan Berkovits


==Description==

The program simulates a small shell of the LINUX operating system,
which implements parts of the original shell:
prompt,
 ; , 
variables,
pipe,
ctrlZ-bg,
&,
>.
Program DATABASE:
database struct
which contains a linked list.
1.variable : A variable that has a name and a value.
functions:
main functions:
1.my_strtok- A function that works like strtok but with slight changes that consider the edge case.
2.create_var- A function that initializes a new variable. 
3.findVar- A function to find the value of a variable from a linked list.
4.saveVar- A function that saves the variable into an element in the linked list,
 it receives a command that contains the placement of the variable.
5.freeVariable- A function that frees the dynamically allocated memory
6.inputCommand-A function that receives input from the user.
7.delete_char_at_index- A function that deletes a character from a string at a certain index.
8.findIndex- A function that finds the index of the first specific character in a string.
9.catch_ctrlZ- A function that is done when a ctrl z signal enters.
10.insert_char_at_index- A function that inserts a character into a certain index.
11.catchChild -This function is invoked when the status of the child process changes.

==Program Files==
ex2.c
==How to compile?==
compile: gcc ex2.c -o ex2
run: ./ex2

==Input:==
User input

==Output:==
shell commands

