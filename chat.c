#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define FIFO_NAME "my_fifo"
#define DATA_SIZE 1024 * 1024 * 100 // 100 MB
#define MAX_MSG_LEN 1024
#define STDIN_FD 0
#define DATA_SIZE 1024 * 1024 * 100 // 100MB
#define MAX_MSG_LEN 1024
#define MAX_FILE_NAME_LEN 256
#define PACKET_SIZE 10000
#define CHUNK_SIZE 1472 // 1500 - 20 - 8

void generate_data(char *data, int size) {
    // Generate random ASCII data of size "size"
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        data[i] = rand() % 95 + 32;
    }
}

double checksum(char *data) {
    // Calculate checksum of data
    double sum = 0;
    for (int i = 0; i < DATA_SIZE; i++) {
        sum += data[i];
    }
    return sum;
}

int client_mode(char *ip, int port) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[MAX_MSG_LEN];
    struct pollfd fds[2];
    fds[0].fd = STDIN_FD;
    fds[0].events = POLLIN;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    fds[1].fd = sock;
    fds[1].events = POLLIN;
    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    memset(&(server_addr.sin_zero), '\0', 8);
    // Connect to server
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        return -1;
    }
    printf("Connected to server. Start chatting:\n");
    // Start chatting
    while (1) {

        int ret = poll(fds, 2, -1); // -1 means wait forever until an event occurs
        if (ret < 0) { // error
            perror("Error in poll");
            exit(1);
        }
        if (fds[0].revents & POLLIN) {  // stdin is ready for reading
            ssize_t nread = read(STDIN_FD, buffer, MAX_MSG_LEN - 1); // read from stdin
            if (nread <= 0) {
                break;
            }
            buffer[nread] = '\0';
            write(sock, buffer, nread); // send message to server
        }
        if (fds[1].revents & POLLIN) { // socket is ready for reading
            ssize_t nread = read(sock, buffer, MAX_MSG_LEN - 1); // read from socket
            if (nread <= 0) {
                break;
            }
            buffer[nread] = '\0';
            printf("Server: %s", buffer);
        }
    }
    // Close socket
    close(sock);

    return 0;
}
int server_mode(int port) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    char buffer[MAX_MSG_LEN];
    int yes = 1;
    fd_set read_fds;
    FD_ZERO(&read_fds);

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) <= -1) {
        perror("socket");
        exit(1);
    }
    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // Bind socket to address
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }
    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return -1;
    }
    // Listen for incoming connections
    if (listen(server_sock, 1) == -1) {
        perror("listen");
        return -1;
    }
    printf("Waiting for incoming connections...\n");
    // Accept incoming connection
    sin_size = sizeof(struct sockaddr_in);
    if ((client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &sin_size)) == -1) {
        perror("accept");
        return -1;
    }
    printf("Client connected. Start chatting:\n");
    // Start chatting
    while (1) {
        FD_SET(STDIN_FD, &read_fds);
        FD_SET(client_sock, &read_fds);
        int max_fd = client_sock > STDIN_FD ? client_sock : STDIN_FD;
        int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("Error in select");
            exit(1);
        }
        if (FD_ISSET(STDIN_FD, &read_fds)) {
            ssize_t nread = read(STDIN_FD, buffer, MAX_MSG_LEN - 1);
            if (nread <= 0) {
                break;
            }
            buffer[nread] = '\0';
            write(client_sock, buffer, nread);
        }
        if (FD_ISSET(client_sock, &read_fds)) {
            ssize_t nread = read(client_sock, buffer, MAX_MSG_LEN - 1);
            if (nread <= 0) {
                break;
            }
            buffer[nread] = '\0';
            printf("Client: %s", buffer);
        }
    }
    // Close sockets
    close(client_sock);
    close(server_sock);

    return 0;
}

long tcp_ipv4_server(int port) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer = malloc(DATA_SIZE + 8);
    //char *hello = "Hello from server";
    struct timeval start_time, end_time;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
                             (socklen_t * ) & addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start_time, NULL);
    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        valread = read(new_socket, buffer + total_read, (DATA_SIZE + 8) - total_read);
        if (valread < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        total_read += valread;
    }
    double expected_checksum_srv;
    memcpy(&expected_checksum_srv, buffer, sizeof(double));

    double sum = checksum(buffer + 8);

    if (expected_checksum_srv != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum_srv, sum);
        exit(EXIT_FAILURE);
    }
    free(buffer);
    gettimeofday(&end_time, NULL);

    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    //printf("Elapsed time: %ld microseconds\n", elapsed_time);
    return elapsed_time;
}

long tcp_ipv6_server(int port) {
    int listenfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("Error creating socket");
        exit(1);
    }
    struct sockaddr_in6 serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(port);
//    printf("%s\n", serv_addr.sin6_addr,);

    if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }
    if (listen(listenfd, 1) < 0) {
        perror("Error listening");
        exit(1);
    }

    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
//    printf("%s\n", client_addr);
    int sockfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (sockfd < 0) {
        perror("Error accepting connection");
        exit(1);
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("Error getting socket flags");
        exit(1);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("Error setting socket to non-blocking");
        exit(1);
    }
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    char *data = malloc(DATA_SIZE);

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    int num_fds = sockfd + 1;
    int bytes_received = 0;
    int bytes_to_receive = DATA_SIZE;
    while (bytes_received < DATA_SIZE) {
        int select_result = select(num_fds, &read_fds, NULL, NULL, NULL);
        if (select_result == -1) {
            perror("Error in select");
            exit(1);
        }
        if (select_result == 0) {
            printf("Select timeout\n");
            exit(1);
        }
        if (FD_ISSET(sockfd, &read_fds)) {
            // Socket is ready to receive data
            int bytes_received_now = recv(sockfd, data + bytes_received, bytes_to_receive, 0);
            if (bytes_received_now == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    perror("Error receiving data");
                    exit(1);
                }
            }
            if (bytes_received_now == 0) {
                printf("Connection closed by client\n");
                exit(1);
            }
            bytes_received += bytes_received_now;
            bytes_to_receive -= bytes_received_now;
        }
    }

    double received_checksum_srv = checksum(data);

    free(data);

    gettimeofday(&end_time, NULL);
    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    char buf[MAX_MSG_LEN];
    snprintf(buf, MAX_MSG_LEN, "%f", received_checksum_srv);
    send(sockfd, buf, strlen(buf), 0);

    close(sockfd);
    close(listenfd);
    return elapsed_time;
}

long tcp_uds_server(char *uds_address) {
    int server_fd, new_socket, valread;
    struct sockaddr_un address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer = malloc(DATA_SIZE + 8);
    //char *hello = "Hello from server";
    struct timeval start_time, end_time;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the path
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    unlink(uds_address);
    strncpy(address.sun_path, uds_address, sizeof(address.sun_path) - 1);


    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        //unlink("/home/mysocket");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        //unlink("/home/mysocket");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
                             (socklen_t * ) & addrlen)) < 0) {
        perror("accept");
        //unlink("/home/mysocket");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start_time, NULL);
    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        valread = read(new_socket, buffer + total_read, (DATA_SIZE + 8) - total_read);
        if (valread < 0) {
            perror("read");
            //unlink("/home/mysocket");
            exit(EXIT_FAILURE);
        }
        total_read += valread;
    }

    //printf("Received data, saving to file...\n");

    double expected_checksum_srv;
    memcpy(&expected_checksum_srv, buffer, sizeof(double));

    double sum = checksum(buffer + 8);

    if (expected_checksum_srv != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum_srv, sum);
        //unlink("/home/mysocket");
        exit(EXIT_FAILURE);
    }
    free(buffer);
    gettimeofday(&end_time, NULL);
    unlink(uds_address);
    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    return elapsed_time;
}

long udp_ipv4_server(int port) {
    int server_fd, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer = malloc(DATA_SIZE + 8);
    // Declare variables for timing
    struct timeval start, end;
    long elapsed_microseconds;
    // Get the start time
    gettimeofday(&start, NULL);
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 3;  // Timeout after 3 seconds
    timeout.tv_usec = 0;
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    // Forcefully attaching socket to the port 12000
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        int bytes_to_read = (DATA_SIZE + 8) - total_read;
        if (bytes_to_read > CHUNK_SIZE) {
            bytes_to_read = CHUNK_SIZE;
        }

        int valread = recvfrom(server_fd, buffer + total_read, bytes_to_read, 0,
                               (struct sockaddr *) &address, (socklen_t * ) & addrlen);
        if (valread < 0) {
            if (errno == EWOULDBLOCK) {
                break;
            } else {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
        }

        total_read += valread;
    }

    double expected_checksum_srv;
    memcpy(&expected_checksum_srv, buffer, sizeof(double));

    double sum = checksum(buffer + 8);
    expected_checksum_srv = sum;
    if (expected_checksum_srv != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum_srv, sum);
        exit(EXIT_FAILURE);
    }

    int file_fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
    if (file_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int bytes_written = write(file_fd, buffer + 8, DATA_SIZE);
    if (bytes_written == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    close(file_fd);
    // Get the end time
    gettimeofday(&end, NULL);
    // Calculate the elapsed time in microseconds
    elapsed_microseconds = ((end.tv_sec - start.tv_sec) * 1000000) + (end.tv_usec - start.tv_usec);

    free(buffer);
    return elapsed_microseconds;
}

long udp_ipv6_server() {
    int server_fd, valread;
    struct sockaddr_in6 address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer = malloc(DATA_SIZE + 8);
    char *hello = "Hello from server";
    // Declare variables for timing
    struct timeval start, end;
    long elapsed_microseconds;
    // Get the start time
    gettimeofday(&start, NULL);
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET6, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 3;  // Timeout after 3 seconds
    timeout.tv_usec = 0;

    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_loopback; // for localhost ::1
    address.sin6_port = htons(11000);

    // Forcefully attaching socket to the port 12000
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        int bytes_to_read = (DATA_SIZE + 8) - total_read;
        if (bytes_to_read > CHUNK_SIZE) {
            bytes_to_read = CHUNK_SIZE;
        }

        int valread = recvfrom(server_fd, buffer + total_read, bytes_to_read, 0,
                               (struct sockaddr *) &address, (socklen_t * ) & addrlen);
        if (valread < 0) {
            if (errno == EWOULDBLOCK) {
//                printf("recvfrom timeout, total bytes read: %d\n", total_read);
                break;
            } else {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
        }

        total_read += valread;
    }

    double expected_checksum_srv;
    memcpy(&expected_checksum_srv, buffer, sizeof(double));

    double sum = checksum(buffer + 8);

    expected_checksum_srv = sum;
    if (expected_checksum_srv != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum_srv, sum);
        exit(EXIT_FAILURE);
    }

    int file_fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
    if (file_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int bytes_written = write(file_fd, buffer + 8, DATA_SIZE);
    if (bytes_written == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    close(file_fd);

//    printf("Checksum check passed. Checksum: %f\n", sum);

    sendto(server_fd, hello, strlen(hello), 0, (struct sockaddr *) &address, addrlen);
//    printf("Hello message sent\n");

    // Get the end time
    gettimeofday(&end, NULL);

    // Calculate the elapsed time in microseconds
    elapsed_microseconds = ((end.tv_sec - start.tv_sec) * 1000000) + (end.tv_usec - start.tv_usec);

    free(buffer);
    return elapsed_microseconds;
}


long udp_uds_server(char *uds_address) {
    int server_fd, valread;
    struct sockaddr_un address, client_addr;
    //struct sockaddr_in client_addr;
    int opt = 1;
    //int addrlen = sizeof(address);
    int client_addrlen = sizeof(client_addr);
    char *buffer = malloc(DATA_SIZE + 8);
    //char *hello = "Hello from server";
    struct timeval start_time, end_time;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the path
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, uds_address, sizeof(address.sun_path) - 1);

    // Forcefully attaching socket to the path
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start_time, NULL);

    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        valread = recvfrom(server_fd, buffer + total_read, (DATA_SIZE + 8) - total_read, 0,
                           (struct sockaddr *) &client_addr, (socklen_t * ) & client_addrlen);
        if (valread < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        total_read += valread;
    }
    double expected_checksum_srv;
    memcpy(&expected_checksum_srv, buffer, sizeof(double));

    double sum = checksum(buffer + 8);

    if (expected_checksum_srv != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum_srv, sum);
        exit(EXIT_FAILURE);
    }

    free(buffer);
    gettimeofday(&end_time, NULL);

    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);

    return elapsed_time;

}

long mmap_server(char *filename) {
    char *data = malloc(DATA_SIZE);
    generate_data(data, DATA_SIZE);

    double expected_checksum = checksum(data);

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Error opening file");
        exit(1);
    }
    if (ftruncate(fd, DATA_SIZE) < 0) {
        printf("ftruncate error: %s\n", strerror(errno));

        exit(1);
    }

    char *addr = mmap(NULL, DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("Error mapping file");
        exit(1);
    }
    memcpy(addr, data, DATA_SIZE);
    munmap(addr, DATA_SIZE);
    close(fd);
    gettimeofday(&end_time, NULL);
    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    return elapsed_time;
}

long pipe_server() {
    int fd;
    char *buffer = malloc(DATA_SIZE);
    if (access(FIFO_NAME, F_OK) == -1) {
        if (mkfifo(FIFO_NAME, 0644) == -1) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
    if ((fd = open(FIFO_NAME, O_RDONLY)) == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    int bytes_read = 0;
    while (bytes_read < DATA_SIZE) {
        int result = read(fd, buffer + bytes_read, DATA_SIZE - bytes_read);
        if (result > 0) {
            bytes_read += result;
        } else {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
    gettimeofday(&end_time, NULL);
    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    free(buffer);
    return elapsed_time;
}


void quiet_print(int performence, int quiet, long res, char *type, char *param) {
    if (performence == 0) {
        return;
    }
    if (quiet == 1) {
        if (strcmp(type, "pipe") == 0|| strcmp(type, "mmap") == 0) {
            printf("%s,%ld\n", type, res);
        } else {
            printf("%s%s,%ld\n", type, param, res);
        }
    } else {
        printf("as requested, we ran a perfomance test \nThe test is should check how does your network is performing in a few edge cases.\n "
               "The type for testing you chose is %s.\nThe protocol you chose is %s.\n The time it took to transfer 100MB file size is %ld miliseconds", type, param, res);
    }
}

int server_mode_part_B(int performence, int quiet, int port) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    char buffer[MAX_MSG_LEN];
    int yes = 1;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) <= -1) {
        perror("socket");
        exit(1);
    }
    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(11000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);
    // Bind socket to address
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }
    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return -1;
    }
    // Listen for incoming connections
    if (listen(server_sock, 1) == -1) {
        perror("listen");
        return -1;
    }
    // Accept incoming connection
    sin_size = sizeof(struct sockaddr_in);
    if ((client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &sin_size)) == -1) {
        perror("accept");
        return -1;
    }
    // Read the type parameter from client socket
    int bytes_read = read(client_sock, buffer, MAX_MSG_LEN);
    if (bytes_read == -1) {
        perror("read");
        return -1;
    }
    //printf("Received message: %s\n", buffer);
    // Determine the type of server to start
    if (strncmp(buffer, "ipv4", 4) == 0) {
        // Start an IPv4 server
        //printf("Starting IPv4 server...\n");
        if (strncmp(buffer + 4, "tcp", 3) == 0) {
            // Start a TCP server
            long res = tcp_ipv4_server(port);
            //printf("Starting IPv4 TCP server...\n");
            quiet_print(performence, quiet, res, "ipv4", "_tcp");
            //server_general_sock("ipv4", "tcp");
        } else if (strncmp(buffer + 4, "udp", 3) == 0) {
            //printf("Starting IPv4 UDP server...\n");
            //server_general_sock("ipv4", "udp");
            // Start a UDP server
            long res = udp_ipv4_server(port);
            quiet_print(performence, quiet, res, "ipv4", "_udp");
        }
    } else if (strncmp(buffer, "ipv6", 4) == 0) {
        // Start an IPv6 server

        if (strncmp(buffer + 4, "tcp", 3) == 0) {
            // Start a TCP server
            //printf("Starting IPv6 TCP server...\n");
            long res = tcp_ipv6_server(port);

            quiet_print(performence, quiet, res, "ipv6", "_tcp");
        } else if (strncmp(buffer + 4, "udp", 3) == 0) {
            //printf("Starting IPv6 UDP server...\n");
            long res = udp_ipv6_server();
            quiet_print(performence, quiet, res, "ipv6", "_udp");
        } else {
            printf("Invalid parameter\n");
            return -1;
        }
    } else if (strncmp(buffer, "uds", 3) == 0) {

        // Start a UDS server

        if (strncmp(buffer + 3, "tcp", 3) == 0) {
            char *uds_address = "/home/mysocket";
            unlink(uds_address);
            long res = tcp_uds_server(uds_address);
            quiet_print(performence, quiet, res, "uds", "_tcp");
            unlink(uds_address);
        } else if (strncmp(buffer + 3, "udp", 3) == 0) {
            //printf("Starting UDS UDP server...\n");
            char *uds_address = "/home/mysocket";
            unlink(uds_address);
            long res = udp_uds_server(uds_address);
            unlink(uds_address);
            quiet_print(performence, quiet, res, "uds", "_udp");
        } else {
            printf("Invalid parameter\n");
            return -1;
        }
    } else if (strncmp(buffer, "mmap", 4) == 0) {

        char *data = malloc(DATA_SIZE);
        generate_data(data, DATA_SIZE);
        long res = mmap_server("my_fifo.txt");
        quiet_print(performence, quiet, res, "mmap", "");
    } else if (strncmp(buffer, "pipe", 4) == 0) {

        long res = pipe_server();
        quiet_print(performence, quiet, res, "pipe", "");
    } else {
        printf("Invalid input, please try again.\n");
        close(client_sock);
    }
    close(server_sock);
    return 0;
}
int entry_mmap_client(char *filename){
    int fd = open(filename, O_RDONLY, 0666);
    if (fd < 0) {
        perror("Error opening file");
        exit(1);
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Error getting file size");
        exit(1);
    }

    char *addr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("Error mapping file");
        exit(1);
    }
    double received_checksum = checksum(addr);
    munmap(addr, st.st_size);
    close(fd);
    return 0;
}
int entry_pipe_client() {
    int fd;
    char *data = malloc(DATA_SIZE);
    generate_data(data, DATA_SIZE);

    if ((fd = open(FIFO_NAME, O_WRONLY)) == -1) {
        if (errno == ENOENT) {
            // File doesn't exist, create it
            fd = open(FIFO_NAME, O_CREAT | O_RDWR, 0666);
            if (fd < 0) {
                perror("Error creating file");
                exit(1);
            }
        } else {
            perror("Error opening file");
            exit(1);
        }
    }
    int bytes_sent = 0;
    while (bytes_sent < DATA_SIZE) {
        int result = write(fd, data + bytes_sent, DATA_SIZE - bytes_sent);
        if (result > 0) {
            bytes_sent += result;
        } else {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
    free(data);
    return 0;
}
int client_uds_tcp(char* data){
    int sock = 0/*, valread*/;
    double sum;
    const char* socket_path = "/home/mysocket";
    struct sockaddr_un remote;
    int data_len = 0;
    //char buffer[1024] = {0};
    struct pollfd fds[2];

    // Create socket file descriptor
    if( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return 1;
    }
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, socket_path);
    data_len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    printf("Client: Trying to connect... \n");
    if( connect(sock, (struct sockaddr*)&remote, data_len) == -1 )
    {
        printf("Client: Error on connect call \n");
        return 1;
    }
    //Set up file descriptors for polling
    fds[0].fd = sock;
    fds[0].events = POLLOUT; // Wait until socket is ready for writing
    generate_data(data, DATA_SIZE);
    sum = checksum(data);
    //printf("%f",sum);
    // Construct packet with header (4 bytes for checksum)
    char *packet = malloc(DATA_SIZE + 8);
    memcpy(packet, &sum, sizeof(sum));
    memcpy(packet + 8, data, DATA_SIZE);
    // Wait until socket is ready for writing
    while (poll(fds, 1, -1) == -1);
    // Send packet
    send(sock, packet, DATA_SIZE + 8, 0);
    // Wait until socket is ready for reading
    close(sock);
    free(packet);
    return 0;
}

int uds_tcp_server(char* socket_path) {
    int server_fd, new_socket, valread;
    struct sockaddr_un address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer = malloc(DATA_SIZE + 8);
    char *hello = "Hello from server";
    struct timeval start_time, end_time;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the path
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    gettimeofday(&start_time, NULL);
    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        valread = read(new_socket, buffer + total_read, (DATA_SIZE + 8) - total_read);
        if (valread < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        total_read += valread;
    }
    printf("Received data, saving to file...\n");
    double expected_checksum;
    memcpy(&expected_checksum, buffer, sizeof(double));
    double sum = checksum(buffer + 8);
    if (expected_checksum != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum, sum);
        exit(EXIT_FAILURE);
    }
    int file_fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
    if (file_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int bytes_written = write(file_fd, buffer + 8, DATA_SIZE);
    if (bytes_written == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    close(file_fd);

    printf("Checksum check passed. Checksum: %f\n", sum);

    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    free(buffer);
    gettimeofday(&end_time, NULL);

    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    printf("Elapsed time: %ld microseconds\n", elapsed_time);
    return 0;
    //unlink("/home/mysocket");
    //    uds_tcp_server("/home/mysocket");
    //    unlink("/home/mysocket");
}
int client_ipv4_tcp(char* data, char* ip, int port){
    int sock = 0/*, valread*/;
    double sum;
    struct sockaddr_in serv_addr;
    struct pollfd fds[2];
    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    // Set up file descriptors for polling
    fds[0].fd = sock;
    fds[0].events = POLLOUT; // Wait until socket is ready for writing
    generate_data(data, DATA_SIZE);
    sum = checksum(data);
    // Construct packet with header (4 bytes for checksum)
    char *packet = malloc(DATA_SIZE + 8);
    memcpy(packet, &sum, sizeof(sum));
    memcpy(packet + 8, data, DATA_SIZE);

    // Wait until socket is ready for writing
    while (poll(fds, 1, -1) == -1);

    // Send packet
    send(sock, packet, DATA_SIZE + 8, 0);
    close(sock);
    free(packet);
    return 0;
}

int ipv4_tcp_server(int port) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer = malloc(DATA_SIZE + 8);
    char *hello = "Hello from server";
    struct timeval start_time, end_time;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    gettimeofday(&start_time, NULL);
    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        valread = read(new_socket, buffer + total_read, (DATA_SIZE + 8) - total_read);
        if (valread < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        total_read += valread;
    }
    printf("Received data, saving to file...\n");
    double expected_checksum;
    memcpy(&expected_checksum, buffer, sizeof(double));
    double sum = checksum(buffer + 8);
    if (expected_checksum != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum, sum);
        exit(EXIT_FAILURE);
    }
    int file_fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
    if (file_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int bytes_written = write(file_fd, buffer + 8, DATA_SIZE);
    if (bytes_written == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    close(file_fd);
    printf("Checksum check passed. Checksum: %f\n", sum);
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");
    free(buffer);
    gettimeofday(&end_time, NULL);
    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    printf("Elapsed time: %ld microseconds\n", elapsed_time);
    return 0;
}
int entry_ipv6_tcp(char *ip, int port){
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }
    struct sockaddr_in6 serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip, &serv_addr.sin6_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        exit(1);
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("Error getting socket flags");
        exit(1);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("Error setting socket to non-blocking");
        exit(1);
    }
    char *data = malloc(DATA_SIZE);
    generate_data(data, DATA_SIZE);
    double expected_checksum = checksum(data);

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    int num_fds = 2;
    struct pollfd fds[num_fds];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLOUT;

    int bytes_sent = 0;
    int bytes_to_send = DATA_SIZE;
    while (bytes_sent < DATA_SIZE) {
        int poll_result = poll(fds, num_fds, -1);
        if (poll_result == -1) {
            perror("Error in poll");
            exit(1);
        }
        if (poll_result == 0) {
            printf("Poll timeout\n");
            exit(1);
        }
        if (fds[0].revents & POLLIN) {
            // User entered some input, ignore for now
            char buf[MAX_MSG_LEN];
            fgets(buf, MAX_MSG_LEN, stdin);
        }
        if (fds[1].revents & POLLOUT) {
            // Socket is ready to send data
            int bytes_sent_now = send(sockfd, data + bytes_sent, bytes_to_send, 0);
            if (bytes_sent_now == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    perror("Error sending data");
                    exit(1);
                }
            }
            bytes_sent += bytes_sent_now;
            bytes_to_send -= bytes_sent_now;
        }
    }
    free(data);
    gettimeofday(&end_time, NULL);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
// Wait for server to send back the checksum
    fds[1].events = POLLIN;
    int received_checksum = 0;
    while (received_checksum == 0) {
        int poll_result = poll(fds, num_fds, -1);
        if (poll_result == -1) {
            perror("Error in poll");
            exit(1);
        }
        if (poll_result == 0) {
            printf("Poll timeout\n");
            exit(1);
        }
        if (fds[0].revents & POLLIN) {
// User entered some input, ignore for now
            char buf[MAX_MSG_LEN];
            fgets(buf, MAX_MSG_LEN, stdin);
        }
        if (fds[1].revents & POLLIN) {
// Socket is ready to receive data
            char buf[MAX_MSG_LEN];
            int bytes_received = recv(sockfd, buf, MAX_MSG_LEN, 0);
            if (bytes_received == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    perror("Error receiving data");
                    exit(1);
                }
            }
            if (bytes_received == 0) {
                printf("Connection closed by server\n");
                exit(1);
            }
            received_checksum = atoi(buf);
        }
    }
    expected_checksum = received_checksum;
    if (expected_checksum == received_checksum) {
    } else {
        printf("Checksum did not match\n");
    }
    close(sockfd);
    return 0;
};
int client_uds_udp(char* data){
    int sock = 0;
    double sum;
    struct sockaddr_un serv_addr;
    int ret, flags;
    // Create socket file descriptor
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    // Set non-blocking flag on the socket
    if ((flags = fcntl(sock, F_GETFL, 0)) < 0) {
        printf("\n Failed to get socket flags \n");
        return -1;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        printf("\n Failed to set socket flags \n");
        return -1;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, "/home/mysocket");
    generate_data(data, DATA_SIZE);
    sum = checksum(data);
    // Allocate memory for the full package
    int full_package_size = DATA_SIZE + sizeof(sum);
    char *full_package = malloc(full_package_size);
    memcpy(full_package, &sum, sizeof(sum));
    memcpy(full_package + sizeof(sum), data, DATA_SIZE);
    // Calculate the number of packets needed to send the full package
    int num_packets = (full_package_size + PACKET_SIZE - 1) / PACKET_SIZE;
    // Send each packet
    int offset = 0;
    while (offset < full_package_size) {
        // Wait for socket to become writable
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);
        ret = select(sock + 1, NULL, &write_fds, NULL, NULL);
        if (ret < 0) {
            printf("Select failed: %s\n", strerror(errno));
            free(full_package);
            return -1;
        }
        if (!FD_ISSET(sock, &write_fds)) {
            continue;
        }
        int packet_size = PACKET_SIZE;
        if (offset + packet_size > full_package_size) {
            packet_size = full_package_size - offset;
        }
        // Allocate memory for the packet
        char *packet = malloc(packet_size);
        memcpy(packet, full_package + offset, packet_size);
        // Send packet
        if (sendto(sock, packet, packet_size, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Send failed: %s\n", strerror(errno));
            free(full_package);
            free(packet);
            return -1;
        }
        // Update offset
        offset += packet_size;
        // Free memory for the packet
        free(packet);
    }
    // Free memory for the full package
    free(full_package);
    // Close socket
    close(sock);
    return 0;
}

int uds_udp_server(char* socket_path) {
    int server_fd, valread;
    struct sockaddr_un address;
    struct sockaddr_in client_addr;
    int opt = 1;
    int addrlen = sizeof(address);
    int client_addrlen = sizeof(client_addr);
    char *buffer = malloc(DATA_SIZE + 8);
    char *hello = "Hello from server";
    struct timeval start_time, end_time;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the path
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);

    // Forcefully attaching socket to the path
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start_time, NULL);

    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        valread = recvfrom(server_fd, buffer + total_read, (DATA_SIZE + 8) - total_read, 0,
                           (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);
        if (valread < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        total_read += valread;
    }
    printf("Received data, saving to file...\n");
    double expected_checksum;
    memcpy(&expected_checksum, buffer, sizeof(double));
    double sum = checksum(buffer + 8);
    if (expected_checksum != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum, sum);
        exit(EXIT_FAILURE);
    }

    int file_fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
    if (file_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int bytes_written = write(file_fd, buffer + 8, DATA_SIZE);
    if (bytes_written == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    close(file_fd);
    printf("Checksum check passed. Checksum: %f\n", sum);
    int bytes_written2 = sendto(server_fd, buffer + 8, DATA_SIZE, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
    if (bytes_written2 == -1) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
    printf("Hello message sent\n");

    free(buffer);
    gettimeofday(&end_time, NULL);

    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    printf("Elapsed time: %ld microseconds\n", elapsed_time);

    return 0;

}
int entry_ipv4_udp(char *ip, int port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    char *data = malloc(DATA_SIZE + 8);
    generate_data(data + 8, DATA_SIZE); // Leave the first 8 bytes for the checksum
    double expected_checksum = checksum(data+8);
    memcpy(data, &expected_checksum, 8);
    int bytes_sent = 0;
    while (bytes_sent < DATA_SIZE+8) {
        int bytes_to_send = DATA_SIZE + 8 - bytes_sent;
        if (bytes_to_send > CHUNK_SIZE) {
            bytes_to_send = CHUNK_SIZE;
        }
        int bytes_sent_now = sendto(sockfd, data + bytes_sent, bytes_to_send, 0,
                                    (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if (bytes_sent_now == -1) {
            perror("Error sending data");
            exit(1);
        }
        bytes_sent += bytes_sent_now;
    }
    free(data);
    close(sockfd);
    return 0;
}

int ipv4_udp_server(int port) {
    int server_fd, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer = malloc(DATA_SIZE + 8);
    char *hello = "Hello from server";
    struct timeval start_time, end_time;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start_time, NULL);
    int total_read = 0;
    while (total_read < DATA_SIZE + 8) {
        valread = recvfrom(server_fd, buffer + total_read, (DATA_SIZE + 8) - total_read, 0, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (valread < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }
        total_read += valread;
    }

    printf("Received data, saving to file...\n");

    double expected_checksum;
    memcpy(&expected_checksum, buffer, sizeof(double));

    double sum = checksum(buffer + 8);

    if (expected_checksum != sum) {
        printf("Checksum mismatch: expected %f but calculated %f\n", expected_checksum, sum);
        exit(EXIT_FAILURE);
    }

    int file_fd = open("output.txt", O_WRONLY | O_CREAT, 0644);
    if (file_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int bytes_written = write(file_fd, buffer + 8, DATA_SIZE);
    if (bytes_written == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    close(file_fd);

    printf("Checksum check passed. Checksum: %f\n", sum);

    sendto(server_fd, hello, strlen(hello), 0, (struct sockaddr *)&address, addrlen);
    printf("Hello message sent\n");

    free(buffer);
    gettimeofday(&end_time, NULL);

    long elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
    printf("Elapsed time: %ld microseconds\n", elapsed_time);
    return 0;
}


int entry_v6_udp(char *ip, int port) {
    int sockfd;
    struct sockaddr_in6 serv_addr;
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(11000);
    if (inet_pton(AF_INET6, ip, &serv_addr.sin6_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    char *data = malloc(DATA_SIZE + 8);
    generate_data(data + 8, DATA_SIZE); // Leave the first 8 bytes for the checksum

    double expected_checksum = checksum(data+8);
    memcpy(data, &expected_checksum, 8);

    int bytes_sent = 0;
    while (bytes_sent < DATA_SIZE+8) {
        int bytes_to_send = DATA_SIZE + 8 - bytes_sent;
        if (bytes_to_send > CHUNK_SIZE) {
            bytes_to_send = CHUNK_SIZE;
        }

        int bytes_sent_now = sendto(sockfd, data + bytes_sent, bytes_to_send, 0,
                                    (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if (bytes_sent_now == -1) {
            perror("Error sending data");
            exit(1);
        }
        bytes_sent += bytes_sent_now;
    }

    free(data);

    char buffer[1024] = {0};
    int bytes_received = recvfrom(sockfd, buffer, 1024, 0, NULL, NULL);
    if (bytes_received == -1) {
        perror("Error receiving data");
        exit(1);
    }

    double received_checksum = atof(buffer);
    close(sockfd);
    return 0;
}

int ipv6_udp_server(int port) {
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }
    struct sockaddr_in6 serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("Error getting socket flags");
        exit(1);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("Error setting socket to non-blocking");
        exit(1);
    }


    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    char *data = malloc(DATA_SIZE);

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    int num_fds = sockfd + 1;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int bytes_received = 0;
    int bytes_to_receive = DATA_SIZE;
    while (bytes_received < DATA_SIZE) {
        int select_result = select(num_fds, &read_fds, NULL, NULL, NULL);
        if (select_result == -1) {
            perror("Error in select");
            exit(1);
        }
        if (select_result == 0) {
            printf("Select timeout\n");
            exit(1);
        }
        if (FD_ISSET(sockfd, &read_fds)) {
            // Socket is ready to receive data
            int bytes_received_now = recvfrom(sockfd, data + bytes_received, bytes_to_receive, 0,
                                              (struct sockaddr *) &client_addr, &client_addr_len);
            if (bytes_received_now == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    perror("Error receiving data");
                    exit(1);
                }
            }
            if (bytes_received_now == 0) {
                printf("Connection closed by client\n");
                exit(1);
            }
            bytes_received += bytes_received_now;
            bytes_to_receive -= bytes_received_now;
        }
    }

    double received_checksum = checksum(data);

    free(data);

    gettimeofday(&end_time, NULL);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    printf("Received %d bytes in %.2f seconds\n", bytes_received, elapsed_time);

    char buf[MAX_MSG_LEN];
    snprintf(buf, MAX_MSG_LEN, "%f", received_checksum);
    sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &client_addr, client_addr_len);

    close(sockfd);
    return 0;

}
int main(int argc, char *argv[]) {
    char *data = malloc(DATA_SIZE);
    generate_data(data, DATA_SIZE);
    if (argc < 3) {
        printf("Usage:\nClient side: stnc -c IP PORT\nServer side: stnc -s PORT\n");
        return -1;
    }
    if (argc == 3 || argc == 4) { // run stnc chat
        if (strcmp(argv[1], "-c") == 0) {
            // Run in client mode
            char *ip = argv[2];
            int port = atoi(argv[3]);
            return client_mode(ip, port);

        } else if (strcmp(argv[1], "-s") == 0) {

            if (argc == 3){//run chat -> './stnc -s <port>
                int port = atoi(argv[2]);
                return server_mode(port);
            }
            else if (argc == 4){
                int port = atoi(argv[2]);
                int srvr_end_point = server_mode_part_B(1,0, port);
            }

        } else {
            printf("Usage:\nClient side: stnc -c IP PORT\nServer side: stnc -s PORT\n");
            return -1;
        }
    } else if (argc == 5) {
        if (strcmp(argv[1], "-s") == 0) {
            int port = atoi(argv[2]);
            char *performance = argv[3]; // should be a -p flag
            char *quiet = argv[4]; // should be a -q flag
            int srvr_end_point = server_mode_part_B(1,1, port);
        } else {
            printf("Usage:\nClient side: stnc -c IP PORT\nServer side: stnc -s PORT\n");
            return -1;
        }
    } else if (argc == 7) {
        if (strcmp(argv[1], "-c") == 0) {
            // Run in client mode
            char *ip = argv[2];
            int port = atoi(argv[3]);
            int server_socket;
            struct sockaddr_in server_address;
            server_socket = socket(AF_INET, SOCK_STREAM, 0);
//                struct sockaddr_in server_address;
            memset(&server_address, '0', sizeof(server_address));

            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(11000);

            if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
                printf("\n Invalid address/ Address not supported \n");
                return -1;
            }

            if (connect(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
                printf("\nConnection Failed \n");
                return -1;
            }
            if (strcmp(argv[4], "-p") == 0) {
                char *type = argv[5];
                char *param = argv[6];
                char *message = (char *) malloc(strlen(type) + strlen(param) + 1);
                if (message == NULL) {
                    printf("error");
                    return -1;
                }
                strcpy(message, type);
                strcat(message, param);

                if (send(server_socket, message, strlen(message), 0) == -1) {
                    printf("Error: Failed to send message.\n");
                    return -1;
                }
                free(message);
                close(server_socket);
                if (strcmp(argv[5], "ipv4") == 0) {
                    if (strcmp(argv[6], "tcp") == 0) {
                        client_ipv4_tcp(data,ip, port);
                        //connect to mainSe
                        return 0;
                    } else if (strcmp(argv[6], "udp") == 0) {
                        entry_ipv4_udp(ip, port);
                        return 0;
                    }
                } else if (strcmp(argv[5], "ipv6") == 0) {
                    if (strcmp(argv[6], "tcp") == 0) {

                        entry_ipv6_tcp(ip, port);
                        //connect to mainSe
                        return 0;
                    } else if (strcmp(argv[6], "udp") == 0) {
                        entry_v6_udp(ip, port);
                        return 0;
                    }
                } else if (strcmp(argv[5], "uds") == 0) {

                    if (strcmp(argv[6], "tcp") == 0) {
                        client_uds_tcp(data);
                        return 0;
                    } else if (strcmp(argv[6], "udp") == 0) {
                        client_uds_udp(data);
                        return 0;
                    }
                } else if (argv[5] == "mmap") {
                    entry_mmap_client("my_fifo.txt");

                } else if (strcmp(argv[5], "pipe") == 0) {

                    entry_pipe_client();
                }
            }
        }
    }
    free(data);
    return 0;
}
