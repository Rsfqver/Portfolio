#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

#define PORT 12345
#define BUF_SIZE 1024

typedef struct {
    int sock;
    char username[50];
    struct sockaddr_in server_addr;
} Client;

Client client;

void sigint_handler(int signum) {
    printf("\nReceived SIGINT, closing connection...\n");
    close(client.sock);
    exit(0);
}

int main() {
    char buffer[BUF_SIZE];
    int pid;

    // 시그널 핸들러 설정
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // 소켓 생성
    client.sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client.sock < 0) {
        perror("socket");
        exit(1);
    }

    // 서버 주소 설정
    memset(&client.server_addr, 0, sizeof(client.server_addr));
    client.server_addr.sin_family = AF_INET;
    client.server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    client.server_addr.sin_port = htons(PORT);

    // 서버에 연결
    if (connect(client.sock, (struct sockaddr *)&client.server_addr, sizeof(client.server_addr)) < 0) {
        perror("connect");
        close(client.sock);
        exit(1);
    }

    // 사용자명 입력
    printf("Welcome to the chat room\n");
    printf("Type 'logout' to exit the chat\n");

    printf("Enter username: ");
    fgets(client.username, sizeof(client.username), stdin);
    client.username[strcspn(client.username, "\n")] = '\0';
    write(client.sock, client.username, strlen(client.username) + 1);

    // 포크하여 읽기 쓰기 처리
    if ((pid = fork()) == 0) {  // 자식 프로세스: 메시지 수신
        char recv_buffer[BUF_SIZE];
        while (1) {
            memset(recv_buffer, 0, BUF_SIZE);
            int n = read(client.sock, recv_buffer, BUF_SIZE);
            if (n <= 0) {
                printf("\nDisconnected from server.\n");
                exit(0);
            }
            // 서버로부터 받은 메시지를 "유저(이름): 메시지" 형식으로 출력
            printf("\r%s\n%s: ", recv_buffer, client.username);
            fflush(stdout);
        }
    } else if (pid > 0) {  // 부모 프로세스: 메시지 전송
        while (1) {
            printf("%s: ", client.username);  // 사용자 이름과 함께 프롬프트 표시
            fgets(buffer, BUF_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strcmp(buffer, "logout") == 0) {  // logout 입력 시 종료
                printf("Logging out...\n");
                write(client.sock, buffer, strlen(buffer) + 1);
                close(client.sock);  // 소켓 닫기
                kill(pid, SIGTERM);  // 자식 프로세스 종료
                break;
            }

            write(client.sock, buffer, strlen(buffer) + 1);
        }
    } else {
        perror("fork");
        close(client.sock);
        exit(1);
    }

    close(client.sock);
    return 0;
}

