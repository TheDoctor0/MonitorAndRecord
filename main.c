#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

enum parts { CPU, GPU, RAM, DISK, MOTHERBOARD, SYSTEM, BIOS };

//"sudo lshw -short | grep system", "r");
//"sudo cat /proc/cpuinfo | grep -E 'vendor|family|model|stepping|MHz|cache|flags'", "r");
//"sudo dmidecode -t baseboard | grep -E 'Manufacturer|Product|Version|Serial'", "r");
//"sudo  lshw -class volume | grep -E 'volume|description|vendor|logical|version|serial|size|capacity|configuration'", "r");
//"sudo dmidecode -t bios | grep -E 'Vendor|Version|Date|Revision'","r");
//"sudo dmidecode -t memory | grep -E 'Size:|Type:|Speed:|Manufacturer|Serial|Asset|Part'", "r");
//"sudo lspci | grep -E -o '.{0,0}VGA.{0,200}'","r");
//"sudo lspci | grep -E -o '.{0,0}Multimedia.{0,200}'","r");

const char commands[][BUFFER_SIZE/4] =
{
    { "lscpu | grep -E 'Architecture|CPU(s)|Vendor ID|Model name|CPU MHz'" },
    { "sudo lspci | grep -E -o '.{0,0}VGA.{0,200}'" },
    { "sudo dmidecode -t memory | grep -E 'Size:|Type:|Speed:|Manufacturer|Serial|Part'" },
    { "sudo lshw -class volume | grep -E 'volume|description|vendor|serial|size|capacity|configuration'" },
    { "sudo dmidecode -t baseboard | grep -E 'Product|Manufacturer|Serial'" },
    { "sudo lshw -c system | grep -E 'product|vendor|serial|'" },
    { "sudo dmidecode -t bios | grep -E 'Vendor|Version|Date|Revision'" }
};

char computer[BIOS][BUFFER_SIZE];

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

void execute_command(const char *command, int type)
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

    //clean(computer[type]);

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
    
    for(int i = 0; i <= BIOS; i++)
    {
        execute_command(commands[i], i);
        printf(computer[i]);
    }

    return 0;
}
