
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/*getting familiar with random variable and urandom*/
int main()
{
    int i;
    unsigned char buffer[100];
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, buffer, 101);
    //buffer now contains the random data
    close(fd);
    for (i = 0; i < 100; ++i)
        printf("%02X", buffer[i]);
    printf("\n");
    return 0;
}
