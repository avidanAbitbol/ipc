#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unistd.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
// define for UDS
#define SOCK_PATH "tpf_unix_sock.server"
#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DATA_SIZE 100 * 1024 * 1024 // 100MB
#define MAXLINE 1024
#define PORT 8080
const int BUFFER_SIZE = MAXLINE * MAXLINE * 100; // 100 MB
#define BACKLOG 10

char globalBuf[MAXLINE]; // shared mem
pthread_mutex_t mutex;
int flag = -1;

char *IP = "127.0.0.1";
clock_t start;
clock_t end;
char *fileName = "file_100MB.txt";
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int create100MBfile(const char *fileName) {
    // Create a buffer to hold the data
    const int BUFFER_SIZE = 100 * 1024 * 1024; // 100MB
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    if (!buffer) {
        perror("malloc");
        return 1;
    }

    // Fill the buffer with random characters
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = rand() % 256;
    }

    // Open the file
    FILE *file = fopen(fileName, "wb");
    if (!file) {
        perror("fopen");
        return 1;
    }

    // Write the data to the file
    size_t bytes_written = fwrite(buffer, sizeof(char), BUFFER_SIZE, file);
    if (bytes_written != BUFFER_SIZE) {
        fprintf(stderr, "Error writing to file\n");
        return 1;
    }

    // Close the file
    if (fclose(file) != 0) {
        perror("fclose");
        return 1;
    }

    // Free the buffer
    free(buffer);
    return 0;
}
int checkSum(char *file_name2)
{
    int f2 = open(file_name2, O_CREAT | O_RDWR);
    int f1 = open(fileName, O_CREAT | O_RDWR);
    // if we had problem to open the files.
    if (f1 == -1)
    {
        perror("open files");
    }
    size_t r;
    long long tmp_sum1;
    char buff[MAXLINE];
    int sum1 = 0;
    while ((r = read(f1, buff, sizeof(buff))) > 0)
    {
        tmp_sum1 = 0;
        for (int i = 0; i < r; i++)
            tmp_sum1 += buff[i];
        bzero(buff, MAXLINE);
        sum1 += tmp_sum1;
    }

    // if we had problem to open the files.
    if (f2 == -1)
    {
        perror("open");
    }

    size_t r2;
    char buff2[MAXLINE];

    int sum2 = 0;
    int tmp_sum2;
    while ((r2 = read(f2, buff2, sizeof(buff2))) > 0)
    {
        tmp_sum2 = 0;
        for (int i = 0; i < r2; i++)
            tmp_sum2 += buff2[i];
        bzero(buff2, MAXLINE);
        sum2 += tmp_sum2;
    }
    close(f1);
    close(f2);

    if (sum2 == sum1)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}
void run_server(char* port) {
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_sockfd, 1);

    char sendline[MAXLINE], recvline[MAXLINE];

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len);

    struct pollfd pfd[2];
    pfd[0].fd = client_sockfd;
    pfd[0].events = POLLIN;
    pfd[1].fd = STDIN_FILENO;
    pfd[1].events = POLLIN;

    while (1) {
        poll(pfd, 2, -1);

        if (pfd[0].revents & POLLIN) {
            int n = read(client_sockfd, recvline, MAXLINE);
            if (n == 0) {
                break;
            }
            printf("Client: %s", recvline);
        }

        if (pfd[1].revents & POLLIN) {
            fgets(sendline, MAXLINE, stdin);
            write(client_sockfd, sendline, strlen(sendline));
        }
    }

    close(client_sockfd);
    close(server_sockfd);
}

void run_client(char* ip, char* port) {
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    connect(client_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    char sendline[MAXLINE], recvline[MAXLINE];

    struct pollfd pfd[2];
    pfd[0].fd = client_sockfd;
    pfd[0].events = POLLIN;
    pfd[1].fd = STDIN_FILENO;
    pfd[1].events = POLLIN;

    while (1) {
        poll(pfd, 2, -1);

        if (pfd[0].revents & POLLIN) {
            int n = read(client_sockfd, recvline, MAXLINE);
            if (n == 0) {
                break;
            }
            printf("Server: %s", recvline);
        }

        if (pfd[1].revents & POLLIN) {
            fgets(sendline, MAXLINE, stdin);
            write(client_sockfd, sendline, strlen(sendline));
        }
    }

    close(client_sockfd);
}
void runTCPClient(char *ip, char *port, int isIPv6) {
    printf("TCP client running. Server: %s:%s\n", ip, port);

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data to the server with TCP
    int clientSocket;
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = isIPv6 ? AF_INET6 : AF_INET; // Use either IPv6 or IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(ip, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        free(data);
        return;
    }

    // Loop through all the results and connect to the first suitable address
    for (p = res; p != NULL; p = p->ai_next) {
        clientSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientSocket == -1) {
            perror("Failed to create TCP client socket");
            continue;
        }

        if (connect(clientSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close(clientSocket);
            perror("Failed to connect to server");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to connect to server\n");
        freeaddrinfo(res);
        free(data);
        return;
    }

    // Send the data to the server
    ssize_t totalBytesSent = 0;
    ssize_t bytesSent;
    while (totalBytesSent < DATA_SIZE) {
        bytesSent = send(clientSocket, data + totalBytesSent, DATA_SIZE - totalBytesSent, 0);
        if (bytesSent == -1) {
            perror("Failed to send data to server");
            close(clientSocket);
            freeaddrinfo(res);
            free(data);
            return;
        }

        totalBytesSent += bytesSent;
    }

    // Step 4: Report the result
    printf("TCP client: Data sent successfully. Bytes sent: %zd\n", totalBytesSent);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(clientSocket);
    freeaddrinfo(res);
    free(data);
}
void runTCPServer(char *port, int isIPv6) {
    printf("TCP server running on port %s\n", port);

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data with TCP
    // Create a socket for TCP communication
    int serverSocket, clientSocket;
    struct sockaddr_storage serverAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    // Create the server socket
    if (isIPv6) {
        serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
    } else {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (serverSocket == -1) {
        perror("Failed to create TCP server socket");
        free(data);
        return;
    }

    // Set up the server address
    if (isIPv6) {
        struct sockaddr_in6 *serverAddress6 = (struct sockaddr_in6 *)&serverAddress;
        serverAddress6->sin6_family = AF_INET6;
        serverAddress6->sin6_port = htons(atoi(port));
        serverAddress6->sin6_addr = in6addr_any;
    } else {
        struct sockaddr_in *serverAddress4 = (struct sockaddr_in *)&serverAddress;
        serverAddress4->sin_family = AF_INET;
        serverAddress4->sin_port = htons(atoi(port));
        serverAddress4->sin_addr.s_addr = INADDR_ANY;
    }

    // Bind the server socket to the specified port
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Failed to bind TCP server socket");
        close(serverSocket);
        free(data);
        return;
    }

    // Listen for incoming connections
    if (listen(serverSocket, BACKLOG) == -1) {
        perror("Failed to listen for connections");
        close(serverSocket);
        free(data);
        return;
    }

    // Accept connections from clients
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (clientSocket == -1) {
        perror("Failed to accept client connection");
        close(serverSocket);
        free(data);
        return;
    }

    // Receive data from the client
    ssize_t totalBytesReceived = 0;
    ssize_t bytesRead;
    while (totalBytesReceived < DATA_SIZE) {
        bytesRead = recv(clientSocket, data + totalBytesReceived, DATA_SIZE - totalBytesReceived, 0);
        if (bytesRead == -1) {
            perror("Failed to receive data from client");
            close(clientSocket);
            close(serverSocket);
            free(data);
            return;
        } else if (bytesRead == 0) {
            printf("Client disconnected.\n");
            break;
        }

        totalBytesReceived += bytesRead;
    }

    // Step 4: Report the result
    printf("TCP server: Data received successfully. Bytes received: %zd\n", totalBytesReceived);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(clientSocket);
    close(serverSocket);
    free(data);
}
void runUDPClient(char *ip, char *port, int isIPv6) {
    printf("UDP client running. Destination: %s:%s\n", ip, port);

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data with UDP
    // Create a socket for UDP communication
    int clientSocket;
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = isIPv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(ip, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        free(data);
        return;
    }

    // Loop through all the results and create a socket with the first suitable address
    for (p = res; p != NULL; p = p->ai_next) {
        clientSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (clientSocket == -1) {
            perror("Failed to create UDP client socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to create UDP client socket\n");
        freeaddrinfo(res);
        free(data);
        return;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), ipstr, sizeof(ipstr));
    printf("UDP client running. Destination: %s:%s\n", ipstr, port);
    freeaddrinfo(res);

    // Send data to the server
    ssize_t bytesSent = sendto(clientSocket, data, DATA_SIZE, 0, p->ai_addr, p->ai_addrlen);
    if (bytesSent == -1) {
        perror("Failed to send data to server");
        close(clientSocket);
        free(data);
        return;
    }

    // Step 4: Report the result
    printf("UDP client: Data sent successfully. Bytes sent: %zd\n", bytesSent);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(clientSocket);
    free(data);
}
void runUDPServer(char *port, int isIPv6) {
    printf("UDP server running on port %s\n", port);

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data with UDP
    // Create a socket for UDP communication
    int serverSocket;
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = isIPv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        free(data);
        return;
    }

    // Loop through all the results and bind to the first suitable address
    for (p = res; p != NULL; p = p->ai_next) {
        serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (serverSocket == -1) {
            perror("Failed to create UDP server socket");
            continue;
        }

        if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close(serverSocket);
            perror("Failed to bind UDP server socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to bind UDP server socket to any address\n");
        free(data);
        return;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), ipstr, sizeof(ipstr));
    printf("UDP server running on %s:%s\n", ipstr, port);
    freeaddrinfo(res);

    // Receive data from clients
    struct sockaddr_storage clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    ssize_t bytesReceived = recvfrom(serverSocket, data, DATA_SIZE, 0,
                                     (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (bytesReceived == -1) {
        perror("Failed to receive data from client");
        close(serverSocket);
        free(data);
        return;
    }

    // Step 4: Report the result
    printf("UDP server: Data received successfully. Bytes received: %zd\n", bytesReceived);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(serverSocket);
    free(data);
}
void runUDSStreamClient(const char *path) {
    printf("UDS stream client running\n");

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data using UDS (stream)
    // Create a socket for UDS stream communication
    int clientSocket;
    struct sockaddr_un serverAddress;

    // Create the client socket
    clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Failed to create UDS stream client socket");
        free(data);
        return;
    }

    // Set up the server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    strncpy(serverAddress.sun_path, path, sizeof(serverAddress.sun_path) - 1);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Failed to connect to UDS stream server");
        close(clientSocket);
        free(data);
        return;
    }

    // Send data to the server
    ssize_t totalBytesSent = 0;
    ssize_t bytesSent;
    while (totalBytesSent < DATA_SIZE) {
        bytesSent = send(clientSocket, data + totalBytesSent, DATA_SIZE - totalBytesSent, 0);
        if (bytesSent == -1) {
            perror("Failed to send data to UDS stream server");
            close(clientSocket);
            free(data);
            return;
        }

        totalBytesSent += bytesSent;
    }

    // Step 4: Report the result
    printf("UDS stream client: Data sent successfully. Bytes sent: %zd\n", totalBytesSent);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(clientSocket);
    free(data);
}
void runUDSServer(const char *path, int type) {
    printf("UDS server running\n");

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data using UDS (stream or datagram)
    // Create a socket for UDS communication
    int serverSocket, clientSocket;
    struct sockaddr_un serverAddress, clientAddress;

    // Create the server socket
    serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Failed to create UDS server socket");
        free(data);
        return;
    }

    // Set up the server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    strncpy(serverAddress.sun_path, path, sizeof(serverAddress.sun_path) - 1);

    // Bind the server socket to the specified path
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Failed to bind UDS server socket");
        close(serverSocket);
        free(data);
        return;
    }

    // Listen for incoming connections
    if (listen(serverSocket, BACKLOG) == -1) {
        perror("Failed to listen for connections");
        close(serverSocket);
        free(data);
        return;
    }

    // Accept connections from clients
    socklen_t clientAddressLength = sizeof(clientAddress);
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (clientSocket == -1) {
        perror("Failed to accept client connection");
        close(serverSocket);
        free(data);
        return;
    }

    // Receive data from the client
    ssize_t totalBytesReceived = 0;
    ssize_t bytesRead;
    while (totalBytesReceived < DATA_SIZE) {

        if (type == 0) {
            bytesRead = recv(clientSocket, data + totalBytesReceived, DATA_SIZE - totalBytesReceived, 0);
        } else if (type == 1) {
            bytesRead = recvfrom(clientSocket, data + totalBytesReceived, DATA_SIZE - totalBytesReceived, 0,
                                 (struct sockaddr *)&clientAddress, &clientAddressLength);
        } else {
            printf("Invalid UDS type. Must be 'stream' or 'datagram'.\n");
            close(clientSocket);
            close(serverSocket);
            free(data);
            return;
        }

        if (bytesRead == -1) {
            perror("Failed to receive data from client");
            close(clientSocket);
            close(serverSocket);
            free(data);
            return;
        } else if (bytesRead == 0) {
            printf("Client disconnected.\n");
            break;
        }

        totalBytesReceived += bytesRead;
    }

    // Step 4: Report the result
    printf("UDS server: Data received successfully. Bytes received: %zd\n", totalBytesReceived);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(clientSocket);
    close(serverSocket);
    free(data);
}
void runMmapServer(const char *filename) {
    printf("Memory-mapped file server running\n");

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data using memory-mapped file
    int fd;
    struct stat fileStat;
    unsigned char *fileData;

    // Open the file
    fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Failed to open the file");
        free(data);
        return;
    }

    // Get the file size
    if (fstat(fd, &fileStat) == -1) {
        perror("Failed to get file size");
        close(fd);
        free(data);
        return;
    }

    // Map the file into memory
    fileData = mmap(NULL, fileStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fileData == MAP_FAILED) {
        perror("Failed to map the file into memory");
        close(fd);
        free(data);
        return;
    }

    // Receive data from the memory-mapped file
    memcpy(data, fileData, DATA_SIZE);

    // Step 4: Report the result
    printf("Memory-mapped file server: Data received successfully. Bytes received: %d\n", DATA_SIZE);
    printf("Checksum: %u\n", checksum);

    // Clean up
    munmap(fileData, fileStat.st_size);
    close(fd);
    free(data);
}
void runPipeServer(const char *pipeName) {
    printf("Pipe server running\n");

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data using named pipe
    int fd;
    ssize_t bytesRead;

    // Open the named pipe for reading
    fd = open(pipeName, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open named pipe for reading");
        free(data);
        return;
    }

    // Receive data from the named pipe
    ssize_t totalBytesReceived = 0;
    while (totalBytesReceived < DATA_SIZE) {
        bytesRead = read(fd, data + totalBytesReceived, DATA_SIZE - totalBytesReceived);
        if (bytesRead == -1) {
            perror("Failed to receive data from named pipe");
            close(fd);
            free(data);
            return;
        } else if (bytesRead == 0) {
            printf("No more data in named pipe.\n");
            break;
        }

        totalBytesReceived += bytesRead;
    }

    // Step 4: Report the result
    printf("Pipe server: Data received successfully. Bytes received: %zd\n", totalBytesReceived);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(fd);
    free(data);
}
void runUDSDClient(const char *path, char *isDatagram) {
    printf("UDS %s client running\n", isDatagram ? "datagram" : "stream");

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data using UDS (datagram or stream)
    // Create a socket for UDS communication
    int clientSocket;
    struct sockaddr_un serverAddress;

    // Create the client socket
    clientSocket = socket(AF_UNIX, isDatagram ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Failed to create UDS client socket");
        free(data);
        return;
    }

    // Set up the server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    strncpy(serverAddress.sun_path, path, sizeof(serverAddress.sun_path) - 1);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Failed to connect to UDS server");
        close(clientSocket);
        free(data);
        return;
    }

    // Send data to the server
    ssize_t bytesSent = send(clientSocket, data, DATA_SIZE, 0);
    if (bytesSent == -1) {
        perror("Failed to send data to server");
        close(clientSocket);
        free(data);
        return;
    }

    // Step 4: Report the result
    printf("UDS %s client: Data sent successfully. Bytes sent: %zd\n", isDatagram ? "datagram" : "stream", bytesSent);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(clientSocket);
    free(data);
}
void runMmapClient(const char *fileName) {
    printf("Mmap client running\n");

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data using mmap
    // Open the file in read-only mode
    int fd = open(fileName, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        free(data);
        return;
    }

    // Map the file into memory
    unsigned char *mappedData = (unsigned char *)mmap(NULL, DATA_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (mappedData == MAP_FAILED) {
        perror("Failed to mmap file");
        close(fd);
        free(data);
        return;
    }

    // Copy the data from the mapped region
    memcpy(data, mappedData, DATA_SIZE);

    // Unmap the file
    if (munmap(mappedData, DATA_SIZE) == -1) {
        perror("Failed to munmap file");
        close(fd);
        free(data);
        return;
    }

    // Close the file
    if (close(fd) == -1) {
        perror("Failed to close file");
        free(data);
        return;
    }

    // Step 4: Report the result
    printf("Mmap client: Data transmitted successfully.\n");
    printf("Checksum: %u\n", checksum);

    // Clean up
    free(data);
}
void runPipeClient(const char *pipeName) {
    printf("Pipe client running\n");

    // Step 1: Generate a chunk of data with 100MB size
    unsigned char *data = (unsigned char *)malloc(DATA_SIZE);
    // Generate the data here

    // Step 2: Generate a checksum for the data
    unsigned int checksum = checkSum(data);

    // Step 3: Transmit the data using named pipe
    int fd;
    ssize_t bytesSent;

    // Open the named pipe for writing
    fd = open(pipeName, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open named pipe for writing");
        free(data);
        return;
    }

    // Send data to the named pipe
    bytesSent = write(fd, data, DATA_SIZE);
    if (bytesSent == -1) {
        perror("Failed to send data to named pipe");
        close(fd);
        free(data);
        return;
    }

    // Step 4: Report the result
    printf("Pipe client: Data sent successfully. Bytes sent: %zd\n", bytesSent);
    printf("Checksum: %u\n", checksum);

    // Clean up
    close(fd);
    free(data);
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("Server: %s -s <port> -p -q\n", argv[0]);
        printf("Client: %s -c <ip> <port> -p <type> <param>\n", argv[0]);
        exit(1);
    }

    int isServer = strcmp(argv[1], "-s") == 0;
    int isClient = strcmp(argv[1], "-c") == 0;

    if (isServer && argc >= 4 && strcmp(argv[2], "-p") == 0) {
        char *port = argv[3];
        int performTest = strcmp(argv[4], "-p") == 0;
        int quietMode = strcmp(argv[5], "-q") == 0;

        if (performTest) {
            // Server performance test mode
            // Check for communication type and invoke the appropriate server function
            if (argc >= 7 && strcmp(argv[6], "ipv4_udp") == 0) {
                runUDPServer(port, 0); // IPv4 UDP server
            } else if (argc >= 7 && strcmp(argv[6], "ipv6_udp") == 0) {
                runUDPServer(port, 1); // IPv6 UDP server
            } else if (argc >= 7 && strcmp(argv[6], "ipv4_tcp") == 0) {
                runTCPServer(port,0);
            } else if (argc >= 7 && strcmp(argv[6], "ipv6_tcp") == 0) {
                runTCPServer(port,1);
            } else if (argc >= 7 && strcmp(argv[6], "mmap") == 0) {
                runMmapServer(fileName);
            } else if (argc >= 7 && strcmp(argv[6], "pipe") == 0) {
                runPipeServer("pipename");
            } else if (argc >= 7 && strcmp(argv[6], "uds") == 0) {
                // Check for UDS type (stream or datagram) and invoke the appropriate server function
                if (argc >= 8 && strcmp(argv[7], "stream") == 0) {
                    runUDSServer(port, 0); // UDS stream server
                } else if (argc >= 8 && strcmp(argv[7], "datagram") == 0) {
                    runUDSServer(port, 1); // UDS datagram server

                } else {
                    printf("Invalid communication type for server performance test.\n");
                    exit(1);
                }
            } else {
                printf("Invalid communication type for server performance test.\n");
                exit(1);
            }
        } else if (isClient && argc >= 6 && strcmp(argv[4], "-p") == 0) {
            char *ip = argv[2];
            char *port = argv[3];
            char *type = argv[5];
            char *param = argc >= 7 ? argv[6] : NULL;

            // Client performance test mode
            // Check for communication type and invoke the appropriate client function
            if (strcmp(type, "ipv4_tcp") == 0) {
                int isIPv6 = 0; // Set this flag accordingly
                runTCPClient(ip, port,  isIPv6) ;
            } else if (strcmp(type, "ipv6_tcp") == 0) {
                int isIPv6 = 1; // Set this flag accordingly
                runTCPClient(ip, port, isIPv6);
            } if (strcmp(type, "ipv4_udp") == 0) {
                int isIPv6 = 0; // Set this flag accordingly
                runUDPClient(ip, port, isIPv6);
            } else if (strcmp(type, "ipv6_udp") == 0) {
                int isIPv6 = 1; // Set this flag accordingly
                runUDPClient(ip, port, isIPv6);
            } else if (strcmp(type, "uds_stream") == 0) {
                runUDSDClient(fileName, "stream");
            } else if (strcmp(type, "uds_dgram") == 0) {
                runUDSDClient(fileName, "dgram");
            } else if (strcmp(type, "mmap") == 0) {
                runMmapClient(fileName);
            } else if (strcmp(type, "pipe") == 0) {
                runPipeClient("pipename");
            } else {
                printf("Invalid communication type for client performance test.\n");
                exit(1);
            }
        } else if (isClient && argc >= 4) {
            // Running the initial program (no communication type options)
            char *ip = argv[2];
            char *port = argv[3];

            // Implement the logic for the initial program here
            // You can invoke the appropriate server or client function based on the given IP and port
            // For example:
            if (isServer) {
                run_server(port);
            } else {
                run_client(ip, port);
            }
        } else {
            printf("Invalid arguments.\n");
            printf("Usage:\n");
            printf("Server: %s -s <port> -p -q\n", argv[0]);
            printf("Client: %s -c <ip> <port> -p <type> <param>\n", argv[0]);
            exit(1);
        }

        return 0;
    }
}
