#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdarg.h>

#define PORT 12345
#define BUF_SIZE 1024
#define MAX_CLIENTS 1000

typedef struct {
    int client_sock;
    int pipe_read[2];
    int pipe_write[2];
    char nickname[BUF_SIZE];
} ClientInfo;

int server_sock;
ClientInfo clients[MAX_CLIENTS];
int client_count = 0;

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);

    FILE *log_file = fopen("/var/log/chat_server.log", "a");
    if (log_file) {
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fclose(log_file);
    }

    va_end(args);
}

void broadcast_message(const char *message, int exclude_sock) {
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].client_sock != exclude_sock) {
            write(clients[i].client_sock, message, strlen(message) + 1);
        }
    }
}

void handle_client(ClientInfo *client_info) {
    char buffer[BUF_SIZE];

    // 클라이언트 접속 로그
    log_message("%s has login the chat.", client_info->nickname);

    close(client_info->pipe_write[1]); // Close unused pipe write end
    close(client_info->pipe_read[0]);  // Close unused pipe read end

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int n = read(client_info->client_sock, buffer, BUF_SIZE);
        if (n <= 0 || strcmp(buffer, "logout") == 0) {
            break;
        }

        // 채팅 메시지를 "닉네임: 메시지" 형식으로 로그에 기록
        log_message("%s: %s", client_info->nickname, buffer);

        // 파이프와 브로드캐스트 처리
        write(client_info->pipe_write[1], buffer, strlen(buffer) + 1);
        read(client_info->pipe_read[0], buffer, BUF_SIZE);
        broadcast_message(buffer, client_info->client_sock);
    }

    // 클라이언트가 나갈 때 로그 기록
    log_message("%s has logout the chat.", client_info->nickname);

    close(client_info->client_sock);
    exit(0);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    if (setsid() < 0) exit(EXIT_FAILURE);
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
}

void sigint_handler(int signum) {
    log_message("Received SIGINT, shutting down...");
    close(server_sock);
    exit(0);
}

void sigterm_handler(int signum) {
    log_message("Received SIGTERM, shutting down...");
    close(server_sock);
    exit(0);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    daemonize();

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        exit(1);
    }

    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        if (client_count < MAX_CLIENTS) {
            ClientInfo *client_info = &clients[client_count++];
            client_info->client_sock = client_sock;

            pipe(client_info->pipe_read);
            pipe(client_info->pipe_write);

            int n = read(client_sock, client_info->nickname, BUF_SIZE);
            if (n <= 0) {
                close(client_sock);
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                close(client_sock);
                continue;
            } else if (pid == 0) {
                close(server_sock);
                handle_client(client_info);
            } else {
                close(client_sock);
                close(client_info->pipe_read[1]);
                close(client_info->pipe_write[0]);
            }
        } else {
            log_message("Maximum number of clients reached.");
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}

