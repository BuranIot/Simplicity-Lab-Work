#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <windows.h>

/* ---- Must match uart_init(...) on the board ---- */
#define COM_PORT   "\\\\.\\COM6"    /* check Device Manager: JLink CDC UART Port (COMx) */
#define BAUD_RATE  115200
#define BYTE_SIZE  8                /* 5..8 */
#define PARITY     NOPARITY         /* NOPARITY, EVENPARITY, ODDPARITY */
#define STOP_BITS  ONESTOPBIT       /* ONESTOPBIT, TWOSTOPBITS */

#define LINE_MAX   128              /* board's buffer is 128 -> max 127 chars */
#define REPLY_MAX  256

static HANDLE open_port(void)
{
    HANDLE port = CreateFileA(COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (port == INVALID_HANDLE_VALUE) {
        printf("Cannot open %s (error %lu)\n", COM_PORT, GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    /* Start from the port's current settings, then change what we need. */
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(port, &dcb)) {
        printf("GetCommState() failed\n");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }
    dcb.BaudRate     = BAUD_RATE;
    dcb.ByteSize     = BYTE_SIZE;
    dcb.Parity       = PARITY;
    dcb.StopBits     = STOP_BITS;
    dcb.fBinary      = TRUE;
    dcb.fParity      = (PARITY != NOPARITY);
    dcb.fOutxCtsFlow = FALSE;       /* no hardware flow control */
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_ENABLE;
    dcb.fRtsControl  = RTS_CONTROL_ENABLE;
    dcb.fOutX        = FALSE;       /* no XON/XOFF */
    dcb.fInX         = FALSE;
    if (!SetCommState(port, &dcb)) {
        printf("SetCommState() failed\n");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    /* Each ReadFile/WriteFile gives up after 1 s. */
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadTotalTimeoutConstant  = 1000;
    timeouts.WriteTotalTimeoutConstant = 1000;
    if (!SetCommTimeouts(port, &timeouts)) {
        printf("SetCommTimeouts() failed\n");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    /* Throw away anything left over in the buffers. */
    PurgeComm(port, PURGE_RXCLEAR | PURGE_TXCLEAR);

    printf("----------------------------------\n");
    printf("Port      = %s\n", COM_PORT);
    printf("Baud rate = %lu\n", (unsigned long)dcb.BaudRate);
    printf("Byte size = %u\n", dcb.ByteSize);
    printf("Parity    = %u (0=none 1=odd 2=even)\n", dcb.Parity);
    printf("Stop bits = %u (0=1 bit 2=2 bits)\n", dcb.StopBits);
    printf("----------------------------------\n");
    return port;
}

/* Send the whole string with one WriteFile. */
static int send_line(HANDLE port, const char *line)
{
    DWORD len = (DWORD)strlen(line);
    DWORD written = 0;

    if (!WriteFile(port, line, len, &written, NULL) || written != len) {
        printf("WriteFile() failed (%lu of %lu bytes)\n",
               (unsigned long)written, (unsigned long)len);
        return -1;
    }
    return 0;
}

/* Read one byte at a time until '\n' or timeout. Returns the length, or -1. */
static int read_line(HANDLE port, char *buf, int max)
{
    int count = 0;

    while (count < max - 1) {
        char c;
        DWORD got = 0;

        if (!ReadFile(port, &c, 1, &got, NULL)) {
            printf("ReadFile() failed\n");
            return -1;
        }
        if (got == 0) {         /* timeout: no more bytes */
            break;
        }
        if (c == '\n') {
            break;
        }
        if (c == '\r') {
            continue;
        }
        buf[count++] = c;
    }

    buf[count] = '\0';
    return count;
}

int main(void)
{
    char line[LINE_MAX + 1];
    char reply[REPLY_MAX];
    LARGE_INTEGER freq, t0, t1;

    HANDLE port = open_port();
    if (port == INVALID_HANDLE_VALUE) {
        return 1;
    }
    QueryPerformanceFrequency(&freq);

    for (;;) {
        printf("\nMessage to send (q = quit): ");
        if (fgets(line, LINE_MAX, stdin) == NULL) {
            break;
        }
        line[strcspn(line, "\r\n")] = '\0';     /* strip the Enter key */
        if (strcmp(line, "q") == 0) {
            break;
        }
        if (line[0] == '\0') {
            continue;
        }
        strcat(line, "\n");                     /* board ends a message at '\n' */

        QueryPerformanceCounter(&t0);
        if (send_line(port, line) != 0) {
            break;
        }
        int n = read_line(port, reply, sizeof(reply));
        QueryPerformanceCounter(&t1);

        if (n <= 0) {
            printf("No reply (timeout) - check COM port and settings\n");
        } else {
            printf("Board: %s\n", reply);
            printf("PC round trip: %.3f ms (includes USB latency)\n",
                   (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart);
        }
    }

    if (!CloseHandle(port)) {
        printf("CloseHandle() failed\n");
        return 1;
    }
    return 0;
}
