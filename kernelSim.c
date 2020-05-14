#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/user.h>


#define MAX_LINE 80
void clearArgs(char*[(MAX_LINE/2) + 1], char[(MAX_LINE/2) + 1]);    //Clears args to prevent overwriting issues, eg. ls -1 then date = date -1

void clearWord(char[10]);   //Clears word to prevent overwriting issues (word represents a temporary buffer between input and args, more later)

void clearInput(char[(MAX_LINE/2) + 1]);    //Clears input to prevent overwriting issues

void InputToArgs(char[(MAX_LINE/2) + 1], char [10], char*[(MAX_LINE/2) + 1], int[1], int[1]); //Fills in the args array from user input

void HistoryToArgs(char[10][(MAX_LINE/2) + 1], char[(MAX_LINE/2) + 1], char [10], char*[(MAX_LINE/2) + 1], int[1], int[1], int[1], int[1]); //Fills in the args array from history, used when user types in "!!" or "!#"

void FillHistory(char[10][(MAX_LINE/2) + 1], char[(MAX_LINE/2) + 1], int[1]); //Fills history from user input

void RetreiveHistory(char[10][(MAX_LINE/2) + 1], int[1]);   //Retreives history when user types "history"

void Fork(pid_t, char*[(MAX_LINE/2) + 1], int[1], char[(MAX_LINE/2) + 1]); //Creates the fork and executes

int main()
{
    
    char *args[(MAX_LINE/2) + 1];
    
    char input[(MAX_LINE/2) + 1], history[10][(MAX_LINE/2) + 1], word[10]; //Word iterates over input, stopping when a space or enter is detected, it hold one word at a time, places into the current indext of args, then is refilled with the next word until the command is finished.
    
    int should_run = 1;
    
    int commandcount[1]; commandcount[0] = 0; //Used in history, index of current command
    
    int ampersand[1]; ampersand[0] = 0;     //Used when ampersand is detected
    
    int wordindex[1]; wordindex[0] = 0;     //Used to represent the current word of the command we are on, at the end it should equal the total number of words in the command
    
    int historyindex[1]; historyindex[0] = 0; //Represents whether the loaded history should be the previous command (!!) or another command (!#)
    
    pid_t pid;
    
    while (should_run)
        
    {
        
        wordindex[0] = 0;
        
        clearArgs(args, input);
    
        clearInput(input);
        
        printf("osh>");
        
        fgets(input, (MAX_LINE/2) + 1, stdin);
        
        FillHistory(history, input, commandcount);
        
        InputToArgs(input, word, args, wordindex, ampersand);
        
        if (strncmp(input, "exit\n", sizeof(input)) == 0)
            
        {   should_run = 0;
            exit(0);
            return 0;
        }
        
        else
            
            if (strncmp(input, "history\n", sizeof(input)) == 0)
                
                RetreiveHistory(history, commandcount);
        
            else
                
                if (strncmp(input, "!!\n", sizeof(input)) == 0)
                    
                {
                        historyindex[0] = 0; //Meaning history index is commandcount[0]-1
                        
                        HistoryToArgs(history, input, word, args, wordindex, ampersand, historyindex, commandcount);
                }
        
                else
                    
                    if ((input[0] == '!')&&(input[1] != '!'))
                        
                    {
                        
                        historyindex[0] = 1; //Meaning history index is input[1]
                   
                        HistoryToArgs(history, input, word, args, wordindex, ampersand, historyindex, commandcount);
                        
                    }
        
        //NULL terminating the args array, since clearing it sets the elements to "", which is needed for strcat (concatination) to work.
        if(!ampersand[0])
            args[wordindex[0]] = NULL;
        if(ampersand[0])
            args[wordindex[0]-1] = NULL;
        
        Fork(pid, args, ampersand, input);
        
    }
    
    return 0;
    
}

void clearArgs(char* args[(MAX_LINE/2)-1], char input[(MAX_LINE/2)-1])
{
    for (int j = 0; j < (MAX_LINE/2) + 1; j = j + 1) //Sets args to "", for string concatination function in InputToArgs/HistoryToArgs to work
        args[j] = (char *)malloc(strlen(input)+1);
}

void clearInput(char input[(MAX_LINE/2)-1])
{
    for(int j = 0; j < (MAX_LINE/2) + 1; j++)
        
        input[j] = (char)NULL;
}

void clearWord(char word[10])
{
    for(int j = 0; j < 10; j++)
        
        word[j] = (char)NULL;
}

void InputToArgs(char input [(MAX_LINE/2) + 1], char word[10], char* args[(MAX_LINE/2) + 1], int wordindex[1], int ampersand[1])
{
    clearArgs(args, input);
    
    int srcindex = 0, destindex = 0;
    
    while(input[srcindex-1] !='\n') //While the command is not finished
        
    {
        
        destindex = 0;
        
        clearWord(word);  //Reset word and take the next one
        
        
        while((input[srcindex] != ' ')&&(input[srcindex] != '\n')) //While we are still on the same word
            
        {
            if (input[srcindex] != '&')
            {
                word[destindex] = input[srcindex];
                
                srcindex++;
                
                destindex++;
            }
            else //This is to ensure the ampersand is not added to the args array as it interfers with executing it
            {
                ampersand[0] = 1;
                srcindex++;
            }
            
        }
        
        srcindex++;
        
        strcat(args[wordindex[0]], word);  //Adds the current word to the current args index then increments the args index and resets the word to start again
        wordindex[0]++;
        
    }
    

}

void HistoryToArgs(char history [10][(MAX_LINE/2) + 1], char input[(MAX_LINE/2) + 1], char word[10], char* args[(MAX_LINE/2) + 1], int wordindex[1], int ampersand[1], int historyindex[1], int commandcount[1])
{
    clearArgs(args, input);  //Exactly the same logic as InputToArgs, except the input is history with some minor modifications
    wordindex[0] = 0;
    
    int srcindex = 0, destindex = 0;
    
    int x;
    
    if (historyindex[0] == 0)
        x = commandcount[0]-1;
    else
        x = atoi(&input[1]);
    
    
    if(strncmp(history[x], "", sizeof(history[x])) == 0) //If the user has not entered any commands yet
        
        printf("History is empty!");
    
    else
        
    {
        
        printf("%s", history[x]);
        
        while(history[x][srcindex-1] !='\n')
            
        {
            
            destindex = 0;
            
            clearWord(word);
            
            while((history[x][srcindex] != ' ')&&(history[x][srcindex] != '\n'))
                
            {
                
                if (history[x][srcindex] != '&')
                {
                    word[destindex] = history[x][srcindex];
                    
                    srcindex++;
                    
                    destindex++;
                }
                else
                {
                    ampersand[0] = 1;
                    srcindex++;
                }
                
            }
            
            srcindex++;
            
            strcat(args[wordindex[0]], word);
            
            wordindex[0]++;
            
        }
    }
    
}

void FillHistory(char history [10][(MAX_LINE/2) + 1], char input [(MAX_LINE/2) + 1], int commandcount[1])
{
    for (int j = 0; j < sizeof(input); j++)
        
    if ((strncmp(input, "history\n", sizeof(input)) != 0) && (strncmp(input, "exit\n", sizeof(input)) != 0) && (input[0] != '!') && (strncmp(input, "\n", sizeof(input)) != 0)) //We do not store the command history, exit, !!, an empty command (just pressing enter), or !# in history
            
            history[commandcount[0]][j] = input[j];
    
    
    
    if ((strncmp(input, "history\n", sizeof(input)) != 0) && (strncmp(input, "exit\n", sizeof(input)) != 0) && (input[0] != '!') && (strncmp(input, "\n", sizeof(input)) != 0))
        
    {
        commandcount[0]++;
    }
    
}

void RetreiveHistory(char history [10][(MAX_LINE/2) + 1], int commandcount[1])
{
    int e = commandcount[0]-1;  //Start at most recent command
    
    while((e > (commandcount[0]-1)-10)&&(e >= 0)) //Iterates over the 10 previous commands, or if less than 10 commands were entered, then until command 0
    {
        if(history[e][0] == ' ')
        {
            printf("History is empty!");
            e = 0;
        }
        
        else
        {
            printf("%d ", e);
            
            printf("%s",history[e]);
            e--;
        }
        
    }
    
    if (e == 0)
        printf("You have entered less than 10 commands, end of History.");
}

void Fork(pid_t pid, char* args[(MAX_LINE/2) + 1], int ampersand[1], char input[(MAX_LINE/2)+1])
{
    pid = fork();
    
    
    if (pid < 0) //Failed to create child
        
        printf("Error!");
    
    if (pid == 0) //Child
        
    {
        
        execvp(args[0], args);  //Execute
        
        if ((strncmp(input, "history\n", sizeof(input)) != 0) && (strncmp(input, "exit\n", sizeof(input)) != 0) && (input[0] != '!') && (strncmp(input, "\n", sizeof(input)) != 0))
        printf("Command Not Executed.\n"); //If incorrect command, however still stored in history
        exit(0); //If child fails to execute, it should terminate
        
    }
    
    else if (pid > 0) //Parent
        
    {
        
        switch (ampersand[0])
        {
            case 0: {break;} //If no ampersand was detected, continue
            case 1: {wait(NULL); break;} //If an ampersand was detected, wait for child to execute, then continue
        }
        
    }
    

}
