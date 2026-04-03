#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

/* I-Bus frame structure */
#define IBUS_MAX_MSG_LENGTH 47

/* I-Bus device addresses */
#define IBUS_DEVICE_DIA 0x3F   /* Diagnostic */
#define IBUS_DEVICE_IKE 0x80   /* Instrument Cluster */
#define IBUS_DEVICE_CDC 0x18   /* CD Changer (used as simulator) */

/* I-Bus commands */
#define IBUS_CMD_MOD_STATUS_RESP 0x02
#define IBUS_CMD_IKE_IGN_STATUS_RESP 0x11

volatile sig_atomic_t keep_running = 1;

void signal_handler(int signum) {
    keep_running = 0;
}

/**
 * Calculate XOR checksum for I-Bus frame
 * Format: [SRC][LEN][DST][CMD][DATA...][CHECKSUM]
 */
uint8_t calculate_checksum(const uint8_t *frame, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= frame[i];
    }
    return checksum;
}

/**
 * Send raw I-Bus frame to serial port
 */
int send_frame(int fd, const uint8_t *frame, size_t len) {
    ssize_t written = write(fd, frame, len);
    if (written < 0) {
        syslog(LOG_ERR, "Failed to write to serial port: %s", strerror(errno));
        return -1;
    }
    if ((size_t)written != len) {
        syslog(LOG_WARNING, "Partial write: %zd/%zu bytes", written, len);
    }
    return 0;
}

/**
 * Create and send Module Status Response
 * FROM: CDC (0x18) TO: IKE (0x80)
 * Format: [SRC][LEN][DST][CMD][DATA][CHECKSUM]
 */
void send_module_status_response(int fd) {
    uint8_t frame[10];
    
    frame[0] = IBUS_DEVICE_CDC;        /* SRC: CD Changer */
    frame[1] = 0x05;                    /* LEN: 6 bytes (LEN through CMD+DATA) */
    frame[2] = IBUS_DEVICE_IKE;        /* DST: Instrument Cluster */
    frame[3] = IBUS_CMD_MOD_STATUS_RESP; /* CMD: Module Status Response (0x02) */
    frame[4] = 0x01;                    /* DATA1: Module is present */
    frame[5] = 0x00;                    /* DATA2: Status */
    
    /* Calculate and append checksum */
    frame[6] = calculate_checksum(frame, 6);
    
    if (send_frame(fd, frame, 7) == 0) {
        syslog(LOG_INFO, "Sent Module Status Response (7 bytes)");
    }
}

/**
 * Create and send Ignition Status Response
 * FROM: IKE (0x80) TO: Diagnostic (0x3F)
 * This simulates IKE responding with ignition status
 */
void send_ignition_status(int fd) {
    uint8_t frame[10];
    
    frame[0] = IBUS_DEVICE_IKE;        /* SRC: Instrument Cluster (simulated) */
    frame[1] = 0x04;                    /* LEN: 5 bytes */
    frame[2] = IBUS_DEVICE_DIA;        /* DST: Diagnostic */
    frame[3] = IBUS_CMD_IKE_IGN_STATUS_RESP; /* CMD: Ignition Status Response (0x11) */
    frame[4] = 0x03;                    /* DATA: Ignition ON (KL15) */
    
    /* Calculate and append checksum */
    frame[5] = calculate_checksum(frame, 5);
    
    if (send_frame(fd, frame, 6) == 0) {
        syslog(LOG_INFO, "Sent Ignition Status Response (6 bytes)");
    }
}

/**
 * Initialize serial port for I-Bus communication
 * 9600 baud, 8 bits, 2 STOP bits, ODD parity
 */
int init_serial_port(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        syslog(LOG_ERR, "Failed to open %s: %s", port, strerror(errno));
        return -1;
    }

    struct stat st;
    fstat(fd, &st);

    /* Detect PTY (major 136) */
    if (S_ISCHR(st.st_mode) && major(st.st_rdev) == 136) {
        syslog(LOG_INFO, "%s is a PTY, skipping serial configuration", port);
        return fd;
    }

    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    
    /* Get current settings */
    if (tcgetattr(fd, &tio) < 0) {
        syslog(LOG_ERR, "tcgetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    /* Configure for I-Bus */
    tio.c_cflag = B9600 | CS8 | PARENB | PARODD | CSTOPB | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR | IGNBRK;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 0;   /* Non-blocking */
    tio.c_cc[VTIME] = 0;
    
    if (tcflush(fd, TCIFLUSH) < 0) {
        syslog(LOG_ERR, "tcflush failed: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        syslog(LOG_ERR, "tcsetattr failed: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    syslog(LOG_INFO, "Serial port initialized: %s (9600, 8bits, 2STOP, ODD parity)", port);
    return fd;
}

void daemonize(void) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS); /* Parent exits */
    }
    
    /* Child process continues */
    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }
    
    /* Change to root directory */
    chdir("/");
    
    /* Redirect standard file descriptors */
    int devnull = open("/dev/null", O_RDWR);
    if (devnull < 0) {
        perror("open /dev/null");
        exit(EXIT_FAILURE);
    }
    
    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    
    if (devnull > 2) close(devnull);
    
    /* Ignore common signals */
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    
    /* Handle graceful shutdown */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
}

int main(int argc, char *argv[]) {
    const char *port = "/dev/ttyUSB0";
    int foreground = 0;
    
    /* Parse arguments */
    if (argc > 1) {
        if (strcmp(argv[1], "-f") == 0 || strcmp(argv[1], "--foreground") == 0) {
            foreground = 1;
            if (argc > 2) port = argv[2];
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [-f|--foreground] [PORT]\n", argv[0]);
            printf("  -f, --foreground  Run in foreground (for debugging)\n");
            printf("  PORT              Serial port (default: /dev/ttyUSB0)\n");
            printf("\nExample: %s -f /dev/ttyUSB0\n", argv[0]);
            printf("Example (daemon): %s /dev/ttyUSB0\n", argv[0]);
            return EXIT_SUCCESS;
        } else {
            port = argv[1];
        }
    }
    
    /* Initialize syslog */
    openlog("ibus-simulator", LOG_PID, LOG_USER);
    
    /* Daemonize if not in foreground mode */
    if (!foreground) {
        syslog(LOG_INFO, "Starting I-Bus simulator daemon on %s", port);
        daemonize();
        syslog(LOG_INFO, "I-Bus simulator daemon started (PID: %d)", getpid());
    } else {
        fprintf(stderr, "[ibus-simulator] Starting in foreground on %s\n", port);
    }
    
    /* Open serial port */
    int fd = init_serial_port(port);
    if (fd < 0) {
        syslog(LOG_ERR, "Failed to initialize serial port");
        closelog();
        return EXIT_FAILURE;
    }
    
    syslog(LOG_INFO, "I-Bus simulator sending frames every 5 seconds");
    
    /* Main loop */
    int frame_count = 0;
    while (keep_running) {
        /* Send alternating frames every 5 seconds */
        if (frame_count % 2 == 0) {
            send_module_status_response(fd);
        } else {
            send_ignition_status(fd);
        }
        
        frame_count++;
        
        /* Sleep for 5 seconds */
        sleep(5);
    }
    
    syslog(LOG_INFO, "Shutting down I-Bus simulator (sent %d frames)", frame_count);
    
    close(fd);
    closelog();
    
    return EXIT_SUCCESS;
}

