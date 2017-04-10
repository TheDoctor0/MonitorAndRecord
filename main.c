#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

enum parts { CPU, GPU, RAM, DISK, MOTHERBOARD, SYSTEM, BIOS };

const char commands[][BUFFER_SIZE/4] =
{
	{ "lscpu | grep -E 'Architecture|CPU(s)|Vendor ID|Model name|CPU min MHz|CPU max MHz'" },
	{ "sudo lspci | grep -E -o '.{0,0}VGA.{0,200}'" },
	{ "sudo dmidecode -t memory | grep -E 'Size:|Type:|Speed:|Manufacturer|Serial|Part'" },
	{ "sudo lshw -class volume | grep -E 'description|vendor|serial|capacity'" },
	{ "sudo dmidecode -t baseboard | grep -E 'Product|Manufacturer|Serial'" },
	{ "sudo lshw -c system | grep -E 'description|product|vendor|serial|width'" },
	{ "sudo dmidecode -t bios | grep -E 'Vendor|Version|Date|Revision'" }
};

char computer[BIOS][BUFFER_SIZE];

char xml[][BUFFER_SIZE*4];

void make_xml()
{
    xml += "<host>";
    
    for(int i = 0; i <= BIOS; i++)
    {
	switch(i)
	{
            case CPU: xml += "<cpu>" + computer[CPU] + "</cpu>"; break;
            case GPU: xml += "<gpu>" + computer[CPU] + "</gpu>"; break;
            case RAM: xml += "<ram>" + computer[CPU] + "</ram>"; break;
            case DISK: xml += "<disk>" + computer[CPU] + "</disk>"; break;
            case MOTHERBOARD: xml += "<motherboard>" + computer[CPU] + "</motherboard>"; break;
            case SYSTEM: xml += "<system>" + computer[CPU] + "</system>"; break;
            case BIOS: xml += "<bios>" + computer[CPU] + "</bios>"; break;
	}

	printf(computer[i]);
    }
    
    xml += "</host>";
}

void clean(char *target)
{
	const char replacement[][5] = { { "	" }, { "   " }, { "  " }, { "\t" } };

	for(int i = 0; i < 4; i++)
	{
		char buffer[BUFFER_SIZE] = { 0 };
		char *insert_point = &buffer[0];
		const char *tmp = target;
		size_t replacement_len = strlen(replacement[i]);

		while (1) 
		{
			const char *p = strstr(tmp, replacement[i]);

			if (p == NULL) 
			{
				strcpy(insert_point, tmp);
				break;
			}

			memcpy(insert_point, tmp, p - tmp);
			insert_point += p - tmp;

			memcpy(insert_point, "", 0);

		   tmp = p + replacement_len;
		}

		strcpy(target, buffer);
	}
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
		if (strstr(line, "Error") == NULL && strstr(line, "[Empty]") == NULL
		&& strstr(line, "Unknown") == NULL && strstr(line, "No Module") == NULL)
		strcat(computer[type], line);
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
	
	for(int i = 0; i <= BIOS; i++)
	{
		execute_command(commands[i], i);

		switch(i)
		{
			case CPU: printf("------CPU------\n"); break;
			case GPU: printf("------GPU------\n"); break;
			case RAM: printf("------RAM------\n"); break;
			case DISK: printf("------DISK------\n"); break;
			case MOTHERBOARD: printf("------MOTHERBOARD------\n"); break;
			case SYSTEM: printf("------SYSTEM------\n"); break;
			case BIOS: printf("------BIOS------\n"); break;
		}

		printf(computer[i]);
	}
        
        make_xml();
        
        printf(xml);

	return 0;
}