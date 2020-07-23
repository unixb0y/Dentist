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
    struct sockaddr_ctl addr;
    bzero(&addr, sizeof(addr));
    addr.sc_len     = sizeof(addr);
    addr.sc_family  = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_id      = 0;
    addr.sc_unit    = 0;
    
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

    FILE *fp = fopen("corpus.txt", "w");
    if (NULL == fp) err(4);

    while (true) {
        uint8_t buffer[256];
        bzero(buffer,256);

        rc = read(fd,buffer,256);
        if (rc < 0) printf("[ERR] Error reading from socket.\n");

        if (buffer[rc-1] != 3) continue;

        char output[2*(rc-1) + 1];
        char *ptr = &output[0];
        for (int i=0; i<rc-1; i++) ptr += sprintf(ptr, "%02X", buffer[i]);        
        fflush(fp);
        fprintf(fp, "%s\n", output);

        printf("%s\n", output);
    }
}