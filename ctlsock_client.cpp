#include <bootstrap.h>
#include <mach/mach.h>
#include <stdio.h>
#include <unistd.h>
#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/kern_control.h>
#include <sys/kern_event.h>
#include <sys/ioctl.h>
 
int main(const int argc, char **argv)
{
    uint8_t data[] = {0x4d, 0xfc, 0x05, 0x00, 0x00, 0x00, 0x00, 0xfb};

    struct sockaddr_ctl addr;
    bzero(&addr, sizeof(addr));
    addr.sc_len     = sizeof(addr);
    addr.sc_family  = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_id      = 0;
    addr.sc_unit    = 0;
    
    int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd != -1) {
        printf("[OK] Socket creation worked. Socket descriptor: %d\n", fd);
    }

    struct ctl_info info;
    memset(&info, 0, sizeof(info));
    strncpy(info.ctl_name, "com.unixb0y.Dentist", sizeof("com.unixb0y.Dentist"));
    if (ioctl(fd, CTLIOCGINFO, &info)) {
        printf("Could not get ID for kernel control.\n");
        exit(-1);
    }
    addr.sc_id = info.ctl_id;
    addr.sc_unit = 0;
    
    printf("[OK] Found Kernel Control. Name: %s, ID: %d\n", info.ctl_name, info.ctl_id);

    int rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc != -1) {
        printf("[OK] Connection created. Return: %d\n", rc);
    }
    
    //int socket, int level, int option_name, const void *option_value, socklen_t option_len
    rc = setsockopt(fd, SYSPROTO_CONTROL, 123, (char*)&data, sizeof(data));
    if (rc != -1) {
        printf("[OK] Sent SockOpt to Kext.\n");
    }

    //int fildes, const void *buf, size_t nbyte
    // rc = write(fd, (uint8_t*)&data, sizeof(data));
    // if (rc != -1) {
    //     printf("[OK] Wrote to Kext. %lu Bytes.\n", sizeof(data));
    // }

    // int socket, const void *buffer, size_t length, int flags
    rc = send(fd, (uint8_t*)&data, sizeof(data), 0);
    if (rc != -1) {
        printf("[OK] Sent message to Kext of %lu Bytes.\n",  sizeof(data));
    }


    while (true) {
        uint8_t buffer[256];
        bzero(buffer,256);

        rc = read(fd,buffer,255);
        if (rc < 0)
            printf("ERROR reading from socket");

        printf("[OK] Got %d Bytes from Kext. ",rc);
        switch (buffer[rc-1]) {
            case 0x01:
                printf("Type: ctlHandleWrite\n");
                break;
            case 0x02:
                printf("Type: SendHCIRequest\n");
                break;
            case 0x03:
                printf("Type: ReceiveInterruptData\n");
                break;
            default:
                printf("Type: %d\n", buffer[rc-1]);
                break;
        }
        
        for (int i=0; i<rc-1; i++)
            printf("%02X", buffer[i]);
        printf("\n");
    }
}