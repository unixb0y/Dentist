#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <sys/socket.h>
#include <sys/kern_control.h>
#include <sys/kern_event.h>
#include <sys/ioctl.h>

using namespace std;

void colorchar(uint8_t c) {
    if ( c == 0x0 ) {
        printf("\e[38;5;244m");
    }
    else if ( isalpha(c) ) {
        printf("\e[38;5;208m");
    }
    else if ( isdigit(c) ) {
        printf("\e[38;5;226m");
    }   
    else if ( !isprint(c) ) {
        printf("\e[38;5;196m");
    }
    else if ( isspace(c) ) {
        printf("\e[38;5;40m");
    }
    else if ( ispunct(c) ) {
    //  printf("\e[38;5;164m");
        printf("\e[38;5;201m");
    }
    else if ( c < 16 ) {
        printf("\e[0;32m");
    }
    else if ( c >= 16 && c <= 31 ) {
        printf("\e[1;35m");
    }
}

void resetcolor(void) {
    printf("\033[0m");
}

int ahex2int(char a, char b) {
    a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
    b = (b <= '9') ? b - '0' : (b & 0x7) + 9;
    return (a << 4) + b;
}

void format(string str) {
    int string_len = str.size();
    int line_written_chars = 0;

    for (int i=0; i<string_len/2; i++) {
        uint8_t character = (char)ahex2int(str[i*2], str[i*2+1]);
        colorchar(character);
        printf("%02X ", character);
        resetcolor();
        line_written_chars += 3;

        if ( i%4 == 3 ) {
            printf(" ");
            line_written_chars += 1;
        }

        bool need_ascii_now = false;
        if ( i+1 == string_len/2 && i%16 != 15 ) {
            need_ascii_now = true;
            for (int k=0; k<(52-line_written_chars); k++) printf(" ");
        }

        if ( i%16 == 15 || need_ascii_now ) {
            printf("|");
            for (int j=(i-(i%16)); j<(i+1); j++) {
                uint8_t character = (char)ahex2int(str[j*2], str[j*2+1]);
                colorchar(character);
                printf("%c", character > 32 && character < 127 ? (char)character : '.');
                resetcolor();
                if ( j%4 == 3 ) printf("|");
            }
            printf("\n");
            line_written_chars = 0;
        }
    }

    printf("\n");
}

int err(int code) {
    switch (code) {
        case 1: printf("[ERR] Could not create socket.\n");
        case 2: printf("[ERR] Could not find Kernel Control.\n");
        case 3: printf("[ERR] Could not create connection.\n");
        case 4: printf("[ERR] Could not open corpus file.\n");
        default: printf("[ERR] Unknown error code.\n");
    }
    exit(-1);
}

int main(const int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "-log") && strcmp(argv[1], "-monitor"))) {
        printf("Usage: dentist_logger [-log] or\n\
                      [-monitor]\n");
        return -1;
    }

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

    // If it's not monitor, but log,
    // open a file to save the output to.
    FILE *fp;
    if (strcmp(argv[1], "-monitor")) {
        fp = fopen("corpus.txt", "w");
        if (NULL == fp) err(4);
    }

    while (true) {
        uint8_t buffer[256];
        bzero(buffer,256);

        rc = read(fd,buffer,256);
        if (rc < 0) printf("[ERR] Error reading from socket.\n");

        if (buffer[rc-1] != 3) continue;

        char output[2*(rc-1) + 1];
        char *ptr = &output[0];
        for (int i=0; i<rc-1; i++) ptr += sprintf(ptr, "%02X", buffer[i]);        
        
        // Save to file if it's log mode.
        if (strcmp(argv[1], "-monitor")) {
            fflush(fp);
            fprintf(fp, "%s\n", output);
        }

        // Print raw hex string in any case
        printf("%s\n", output);

        // If it's not log, but monitor,
        // output in hexdump style
        if (strcmp(argv[1], "-log")) {
            string str(output);
            format(str);
        }
    }
}