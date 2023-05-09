#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

void check(sum)
{
    if (sum == 0xdead)
    {
        char *buf;
        int fd = open("flag.txt", O_RDONLY);
        read(fd, &buf, 32);
        printf("[+] - Flag: %.*s\n", 32, &buf);
    }
    else
    {
        printf("[-] - Try Again, I know you are can do better.\n");
    }

    fflush(stdout);
}

int main()
{
    setbuf(stdout, 0);
    int i=0;
    int sum = 0xbabe;
    char buffer[1001];
    printf("  _______                      _____ _______ ______ \n");
    printf(" |__   __|                    / ____|__   __|  ____|\n");
    printf("    | | __ _ _ __   __ _ _ __| |       | |  | |__   \n");
    printf("    | |/ _` | '_ \\ / _` | '__| |       | |  |  __|  \n");
    printf("    | | (_| | |_) | (_| | |  | |____   | |  | |     \n");
    printf("    |_|\\__,_| .__/ \\__,_|_|   \\_____|  |_|  |_|  V 2.0\n"); 
    printf("            | |                                     \n");
    printf("            |_|                                     \n");
    printf("======================== PASSWORD =======================\n");
    printf("[?] - Enter Password: ");
    fflush(stdout);
    fgets(buffer, 1000, stdin);
    buffer[1000]='\0';
    while (buffer[i] != '\0')
    {
        if ('A' <= buffer[i] && buffer[i] <= 'Z')
        {
            sum += buffer[i];
        }
        i++;
    }

    check(sum);
}

