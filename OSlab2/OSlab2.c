#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
 
#define SRV_PORT 12345
#define BUF_SIZE 256
#define BACKLOG 5
 
static volatile sig_atomic_t signal_caught = 0;
 
void on_sighup(int s) {
    (void)s; 
    signal_caught = 1;
}
 
int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("server: socket failed");
        exit(EXIT_FAILURE);
    }
 
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("server: setsockopt");
        exit(EXIT_FAILURE);
    }
 
    struct sockaddr_in srv_addr = {0};
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SRV_PORT);
   
    srv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
 
    if (bind(server_sock, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("server: bind error");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
 
    if (listen(server_sock, BACKLOG) < 0) {
        perror("server: listen error");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    struct sigaction act = {0};
    act.sa_handler = on_sighup;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);
 
    if (sigaction(SIGHUP, &act, NULL) < 0) {
        perror("server: sigaction");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
 
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGHUP);
 
    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) < 0) {
        perror("server: sigprocmask");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
 
    int active_conn = -1; 
    char recv_buf[BUF_SIZE];
 
    fprintf(stdout, "Server listening on 127.0.0.1:%d...\n", SRV_PORT);
 
    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_sock, &fds);
        
        int max_desc = server_sock;
 
        if (active_conn != -1) {
            FD_SET(active_conn, &fds);
            if (active_conn > max_desc) {
                max_desc = active_conn;
            }
        }
 
        int events = pselect(max_desc + 1, &fds, NULL, NULL, NULL, &old_set);
 
        if (events < 0) {
            if (errno == EINTR) {
                if (signal_caught) {
                    fprintf(stderr, "[!] SIGHUP received.\n");
                    signal_caught = 0;
                }
                continue;
            } else {
                perror("server: pselect");
                break;
            }
        }
 
        if (FD_ISSET(server_sock, &fds)) {
            struct sockaddr_in cli_addr = {0};
            socklen_t cli_len = sizeof(cli_addr);
            int new_sock = accept(server_sock, (struct sockaddr*)&cli_addr, &cli_len);
            
            if (new_sock < 0) {
                perror("accept failed");
                continue;
            }
 
            if (active_conn == -1) {
                active_conn = new_sock;
                printf("[+] New connection established (fd=%d)\n", active_conn);
            } else {
                close(new_sock); 
            }
        }
 
        if (active_conn != -1 && FD_ISSET(active_conn, &fds)) {
            ssize_t bytes_read = recv(active_conn, recv_buf, BUF_SIZE, 0);
            
            if (bytes_read > 0) {
                printf("[>] Received %zd bytes\n", bytes_read);
            } else if (bytes_read == 0) {
                printf("[-] Client disconnected\n");
                close(active_conn);
                active_conn = -1;
            } else {
                perror("recv error");
                close(active_conn);
                active_conn = -1;
            }
        }
    }
 
    if (active_conn != -1) close(active_conn);
    close(server_sock);
    return 0;
}
