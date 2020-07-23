#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/kern_control.h>
#include <sys/kern_event.h>
#include <sys/ioctl.h>

int err(int code) {
    switch (code) {
        case 1:
            printf("[ERR] Could not create socket.\n");
        case 2:
            printf("[ERR] Could not find Kernel Control.\n");
        case 3:
            printf("[ERR] Could not create connection.\n");
        case 4:
            printf("[ERR] Could not open corpus file.\n");
        default:
            printf("[ERR] Unknown error code.\n");
    }
    exit(-1);
}
 
int main(const int argc, char **argv)
{
    /*
    *  addr is later used to connect to the socket.
    *  https://developer.apple.com/documentation/kernel/sockaddr_ctl
    */

    struct sockaddr_ctl addr;
    bzero(&addr, sizeof(addr));
    addr.sc_len     = sizeof(addr);
    addr.sc_family  = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_id      = 0;
    addr.sc_unit    = 0;

    /*
    *  info is used to find the kernel control ID (ctl_id).
    *  https://developer.apple.com/documentation/kernel/ctl_info
    */

    struct ctl_info info;
    bzero(&info, sizeof(info));
    strncpy(info.ctl_name, "com.unixb0y.Dentist", sizeof("com.unixb0y.Dentist"));
    
    int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (!fd) err(1);
    printf("[OK] Socket creation worked. Socket descriptor: %d\n", fd);

    int res = ioctl(fd, CTLIOCGINFO, &info);
    if (res) err(2);
    addr.sc_id = info.ctl_id;
    printf("[OK] Found Kernel Control. Name: %s, ID: %d\n", info.ctl_name, info.ctl_id);

    int rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc) err(3);
    printf("[OK] Connection created. Return: %d\n", rc);
    
    /*
	* Let's send some data to the chip.
    */

    uint8_t data[] = {0x4d, 0xfc, 0x05, 0x00, 0x00, 0x00, 0x00, 0xfb};

    /*
    *  You can use all sorts of socket operations
    *  to communicate with the Dentist Kext like
    *  setsockopt, but hot all are implemented.
    */

    //int socket, int level, int option_name, const void *option_value, socklen_t option_len
    rc = setsockopt(fd, SYSPROTO_CONTROL, 123, (char*)&data, sizeof(data));
    if (rc != -1) printf("[OK] Sent SockOpt to Kext.\n");

    /*
    *  Just using send and recv syscalls is the
    *  preferred option for now.
    */

    // int socket, const void *buffer, size_t length, int flags
    rc = send(fd, (uint8_t*)&data, sizeof(data), 0);
    if (rc != -1) {
        printf("[OK] Sent message to Kext of %lu Bytes.\n",  sizeof(data));
    }

    /*
    *  Listen for data sent from Dentist.
    *  By default, it sends whatever data
    *  is sent from driver to chip, 
    *  from chip to driver 
    *  and via sockets to Dentist (debug).
    *
    *  Dentist appends a byte to the end of
    *  each message to identify the type.
    */

    uint8_t buffer[256];
    while (true) {
        bzero(buffer,256);

        rc = read(fd,buffer,256);
        if (rc < 0) printf("[ERR] Error reading from socket.\n");

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
        
        for (int i=0; i<rc-1; i++) printf("%02X", buffer[i]);
        printf("\n");
    }
}