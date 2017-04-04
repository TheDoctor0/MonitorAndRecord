#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

enum { CPU, RAM, DISK, GPU, MOTHERBOARD };

char computer[MOTHERBOARD][BUFFER_SIZE];

char *clean(char *str)
{
    char *src, *dest;

    src = dest = str;

    while(*src != '\0')
    {
        if (*src != '\"' && *src != ',' && *src != '\t')
        {
            *dest = *src;
            dest++; 
        }
        src++;
    }
    *dest = '\0';

    while(str = strstr(str, "    ")) memmove(str, str + 4, 1 + strlen(str + 4));

    return str;
}

void execute_command(char *command, int type)
{
    FILE *fpipe;
    char line[BUFFER_SIZE/4];

    if(!(fpipe = (FILE*) popen(command, "r")))
    { 
        perror("Command cannot be executed");
        exit(1);
    }
    
    while(fgets(line, sizeof(char) * BUFFER_SIZE, fpipe))
    {
	if(!computer[type][0]) strncpy(computer[type], line, sizeof(line));
	else strcat(computer[type], line);
    }

    clean(computer[type]);

    pclose(fpipe);
}

int main(int argc, char **argv)
{
    int result = system("lshw &>/dev/null");
    
    if(result != 0)
    {
        printf("lshw package is not installed\n");
        exit(1);
    }
    
    execute_command("sudo lshw -C memory -json | grep -e '\"id\"' -e '\"size\"' -e '\"description\"' -e '\"version\"'", CPU);
    
    printf(computer[CPU]);

    return 0;
}