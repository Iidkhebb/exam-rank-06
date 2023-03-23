#include <libc.h>

// GLOBALS

#define READ_SIZE 65536*2
#define MAX_FD_SIZE 1024

int max_fd = 0;
int clients_ID = 0;
int clients_ARRAY[MAX_FD_SIZE];

fd_set current_set, R_set, W_set;

char write_BUFFER[READ_SIZE];
char read_BUFFER[READ_SIZE];
char strings_ARRAY[MAX_FD_SIZE];

// GLOBALS

void error(char *msg)
{
    write(2, msg, strlen(msg));
    exit(1);
}

void boadcast(int client_fd, char *msg)
{
    for (int i = 3; i <= max_fd; i++)
    {
        if (FD_ISSET(i, &W_set) && i != client_fd)
        {
            send(i, msg, strlen(msg), 0);
        }
    }
}
void register_client(int input_fd)
{
    max_fd = input_fd > max_fd ? input_fd : max_fd;
    clients_ARRAY[input_fd] = clients_ID++;
    FD_SET(input_fd, &current_set);
    sprintf(write_BUFFER, "server: client %d just arrived\n", clients_ARRAY[input_fd]);
    boadcast(input_fd, write_BUFFER);
}



void send_all(int content_len, int fd)
{
    for (int i = 0, j = 0; i < content_len; i++, j++)
    {
        strings_ARRAY[j] = read_BUFFER[j];
        if (strings_ARRAY[j] == '\n')
        {
            strings_ARRAY[j] = '\0';
            sprintf(write_BUFFER, "client %d: %s\n", clients_ARRAY[fd], strings_ARRAY);
            boadcast(fd, write_BUFFER);
            j = -1;
        }
    }
}

void disconnect_client(int fd)
{
    sprintf(write_BUFFER, "server: client %d just left\n", clients_ARRAY[fd]);
    boadcast(fd, write_BUFFER);
    FD_CLR(fd, &current_set);
    close(fd);
}

int main(int argc, char **argv)
{
    int sockfd, connection_fd;
    socklen_t len;
    struct sockaddr_in servaddr;
    if (argc != 2)
    {
        error("Wrong number of arguments\n");
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
        error("Fatal error\n");

    bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(argv[1]));
    
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		error("Fatal error\n");

	if (listen(sockfd, 10) != 0) 
		error("Fatal error\n");

    max_fd = sockfd;
    FD_ZERO(&current_set);
    FD_SET(sockfd, &current_set);

    while (42)
    {
        R_set = W_set = current_set;
        if (select (max_fd + 1, &R_set, &W_set, NULL, NULL) < 0)
            continue;
        
        for (int fd = 3; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &R_set)){
                if (fd == sockfd)
                {
                    connection_fd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
                    if (connection_fd < 0)
                    {
                        continue;;
                    }
                    else
                    {
                        register_client(connection_fd);
                        break;
                    }
                }
                else
                {
                    int count = recv(fd, read_BUFFER, READ_SIZE, 0);
                    if (count <=  0)
                    {
                        disconnect_client(fd);
                        break;
                    }
                    else
                    {
                        send_all(count, fd);
                        break;
                    }
                }
            }
        }
    }
}
