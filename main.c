#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#define BUFFER_SIZE 1024

enum parts { CPU, GPU, RAM, DISK, MOTHERBOARD, SYSTEM, BIOS };

enum monitoring { RAM_TOTAL, RAM_FREE, RAM_COUNTER, DISK_FREE, DISK_COUNTER };

const char commands[][BUFFER_SIZE / 4] = 
{
    // Parts
    { "lscpu | grep -E 'Architecture|CPU(s)|Vendor ID|Model name|CPU min MHz|CPU max MHz'" },
    { "sudo lspci | grep -E -o '.{0,0}VGA.{0,200}'" },
    { "sudo dmidecode -t memory | grep -E 'Size:|Type:|Speed:|Manufacturer|Serial|Part'" },
    { "sudo lshw -class volume | grep -E 'description|vendor|serial|capacity'" },
    { "sudo dmidecode -t baseboard | grep -E 'Product|Manufacturer|Serial'" },
    { "sudo lshw -c system | grep -E 'description|product|vendor|serial|width'" },
    { "sudo dmidecode -t bios | grep -E 'Vendor|Version|Date|Revision'" },
    // Monitoring
    { "grep -E 'MemTotal|MemFree' /proc/meminfo" },
    { "df $PWD | awk '/[0-9]%/{print $(NF-2)}'" }
};

char computer[BIOS + 1][BUFFER_SIZE];
char xml[BUFFER_SIZE * 4];
long int monitoringStatus[DISK_COUNTER + 1];

void clean(char *target) 
{
    const char replacement[][5] = { "    ", "   ", "  ", "\t" };

    for (int i = 0; i < 4; i++) 
    {
        char buffer[BUFFER_SIZE] = {0};
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

void execute_command(const char *command, int type, int part) 
{
    FILE *fpipe;
    char line[BUFFER_SIZE / 2];

    if (!(fpipe = (FILE*) popen(command, "r"))) 
    {
        perror("Command cannot be executed");

        exit(1);
    }

    if(part)
    {
        while (fgets(line, sizeof (char) * BUFFER_SIZE, fpipe)) 
        {
            if (strstr(line, "Error") == NULL && strstr(line, "[Empty]") == NULL
            && strstr(line, "Unknown") == NULL && strstr(line, "No Module") == NULL)
                strcat(computer[type], line);
        }

        clean(computer[type]);
    }
    else
    {
        char *temp, *size;
        long int number;
        int i = 0;

        while (fgets(line, sizeof (char) * BUFFER_SIZE, fpipe)) 
        {
            if(type == RAM)
            {
                temp = strtok(line, ":");

                while(temp != NULL)
                {
                    long int number;

                    if(sscanf(temp, "%li", &number) == 1) monitoringStatus[i++] = number;

                    temp = strtok(NULL, " kB");
                }
            }

            if(type == DISK && sscanf(temp, "%li", &number) == 1) monitoringStatus[DISK_FREE] = number;
	    
        }

        if(type == RAM)
        {
             double freeRAM = (monitoringStatus[RAM_FREE] * 1.0) / (monitoringStatus[RAM_TOTAL] * 1.0) * 100.0;

             if(freeRAM < 5.0)
             {
                 monitoringStatus[RAM_COUNTER]++;
                 printf("RAM usage is high (%f%).\n", 100.0 - freeRAM);
             }

             if(monitoringStatus[RAM_COUNTER] >= 3)
             {
                 monitoringStatus[RAM_COUNTER] = 0;
                 printf("Sending warning...\n");
             }

             printf("Free RAM: %li/%li (%.2f%)\n", monitoringStatus[RAM_FREE], monitoringStatus[RAM_TOTAL], freeRAM);
        }

        if(type == DISK)
        {
            if(monitoringStatus[DISK_FREE] < 1073741824) // 1 GB
            {
                printf("Free space on disk is low: %li bytes.\n", monitoringStatus[DISK_FREE]);
                printf("Sending warning...\n");
            }
        }
    }

    pclose(fpipe);
}

void make_xml() 
{
    strcat(xml, "<host>\n");

    for (int i = 0; i <= BIOS; i++) 
    {
        execute_command(commands[i], i, 1);

        switch (i) 
        {
            case CPU: strcat(xml, "<cpu>\n"); strcat(xml, computer[CPU]); strcat(xml, "</cpu>\n"); break;
            case GPU: strcat(xml, "<gpu>\n"); strcat(xml, computer[GPU]); strcat(xml, "</gpu>\n"); break;
            case RAM: strcat(xml, "<ram>\n"); strcat(xml, computer[RAM]); strcat(xml, "</ram>\n"); break;
            case DISK: strcat(xml, "<disk>\n"); strcat(xml, computer[DISK]); strcat(xml, "</disk>\n"); break;
            case MOTHERBOARD: strcat(xml, "<motherboard>\n"); strcat(xml, computer[MOTHERBOARD]); strcat(xml, "</motherboard>\n"); break;
            case SYSTEM: strcat(xml, "<system>\n"); strcat(xml, computer[SYSTEM]); strcat(xml, "</system>\n"); break;
            case BIOS: strcat(xml, "<bios>\n"); strcat(xml, computer[BIOS]); strcat(xml, "</bios>\n"); break;
        }
    }

    strcat(xml, "</host>\n");
}

void send_xml()
{
    make_xml();

    char *temp = strdup(xml);

    strcpy(xml, "xml=");
    strcat(xml, temp);

    free(temp);

    CURL *curl;
    CURLcode res;
 
    curl = curl_easy_init();
  
    if(curl) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://ewidencja.5v.pl");

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml);
 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(xml));
 
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
 
        curl_easy_cleanup(curl);
    }
}

void monitor_ram()
{
    execute_command(commands[BIOS + 1], RAM, 0);

    sleep(10);

    monitor_ram();
}

void monitor_disks()
{
    execute_command(commands[BIOS + 2], DISK, 0);

    sleep(60);

    monitor_disks();
}

int main(int argc, char *argv[]) 
{
    int result = system("lshw &>/dev/null");

    if (result != 0) 
    {
        printf("lshw package is not installed\n");

        exit(EXIT_FAILURE);
    }

    int opt;

    while ((opt = getopt(argc, argv, "rd")) != -1) 
    {
        switch (opt)
        {
            case 'r': fprintf(stderr, "RAM monitoring enabled.\n"); monitor_ram(); break;
            case 'd': fprintf(stderr, "Disk monitoring enabled.\n"); monitor_disks(); break;
            default: fprintf(stderr, "Usage: %s [-rd]\nr - monitor RAM\nd - monitor disks\n", argv[0]); exit(EXIT_FAILURE);
        }
    }

    send_xml();

    return 0;
}
