#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define PORT 8093
#define BUFFER_SIZE 1024
#define ADDRESS "127.0.0.3"

void handle_client(int client_sock);
void handle_ufile(int client_sock, char *filename, char *dest_path, char *buffer);
void handle_dfile(int client_sock, char *filename);
void handle_rmfile(int client_sock, char *filename);
void handle_dtar(int client_sock, char *filetype);

int main()
{
    printf("hey1");
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, ADDRESS, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 3) < 0)
    {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Spdf server listening on port %d...\n", PORT);

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) >= 0)
    {
        printf("New client connected to Spdf\n");

        if (fork() == 0)
        {
            close(server_sock);
            handle_client(client_sock);
            close(client_sock);
            exit(0);
        }
        else
        {
            close(client_sock);
            wait(NULL);
        }
    }

    if (client_sock < 0)
    {
        perror("Accept failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    close(server_sock);
    return 0;
}

void handle_client(int client_sock)
{
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        memset(command, 0, BUFFER_SIZE);
        memset(arg1, 0, BUFFER_SIZE);
        memset(arg2, 0, BUFFER_SIZE);

        if (recv(client_sock, buffer, BUFFER_SIZE, 0) <= 0)
        {
            perror("recv failed");
            break;
        }

        sscanf(buffer, "%s %s %s", command, arg1, arg2);

        if (strcmp(command, "ufile") == 0)
        {
            handle_ufile(client_sock, arg1, arg2, buffer);
        }
        else if (strcmp(command, "dfile") == 0)
        {
            handle_dfile(client_sock, arg1);
        }
        else if (strcmp(command, "rmfile") == 0)
        {
            handle_rmfile(client_sock, arg1);
        }
        else if (strcmp(command, "dtar") == 0)
        {
            handle_dtar(client_sock, arg1);
        }
        else
        {
            char *msg = "Invalid command\n";
            send(client_sock, msg, strlen(msg), 0);
        }
    }
}

int create_directory_recursive(const char *path)
{
    char temp[256];
    char *p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);

    if (temp[len - 1] == '/')
    {
        temp[len - 1] = '\0';
    }

    for (p = temp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(temp, 0755) != 0 && errno != EEXIST)
            {
                perror("mkdir");
                return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(temp, 0755) != 0 && errno != EEXIST)
    {
        perror("mkdir");
        return -1;
    }

    return 0;
}

void handle_ufile(int client_sock, char *filename, char *dest_path, char *buffer)
{
    char file_path[BUFFER_SIZE];
    FILE *file;
    size_t file_size;
    char response[BUFFER_SIZE];
    char destination_path[BUFFER_SIZE];
    char file_buffer[BUFFER_SIZE];
    int bytes_received;

    snprintf(destination_path, sizeof(destination_path), "/home/garg83/spdf/%s", dest_path);
    if (create_directory_recursive(destination_path) != 0)
    {
        return;
    }
    snprintf(file_path, sizeof(file_path), "/home/garg83/spdf/%s/%s", dest_path, filename);
    struct stat st = {0};

    file = fopen(file_path, "wb");
    if (file == NULL)
    {
        snprintf(response, sizeof(response), "Failed to open file %s for writing\n", file_path);
        send(client_sock, response, strlen(response), 0);
        return;
    }
    printf("Receiving file: %s\n", file_path);
    while ((bytes_received = recv(client_sock, file_buffer, sizeof(file_buffer), 0)) > 0)
    {
        printf("Received %d bytes\n", bytes_received);
        fwrite(file_buffer, 1, bytes_received, file);
        if (bytes_received < sizeof(file_buffer))
        {
            printf("End of file detected\n");
            break; // End of file
        }
    }

    snprintf(response, sizeof(response), "File %s uploaded successfully\n", filename);
    send(client_sock, response, strlen(response), 0);
    fclose(file);
}

void handle_dfile(int client_sock, char *filename)
{
    char buffer[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    int file;
    int file_size;
    char response[BUFFER_SIZE];

    snprintf(file_path, sizeof(file_path), "/home/garg83/spdf/%s", filename);
    printf("File to be uploaded from: %s\n", file_path);
    file = open(file_path, O_RDONLY);

    if (file < 0)
    {
        perror("Failed to open file");
        return;
    }

    while ((file_size = read(file, buffer, sizeof(buffer))) >= 0)
    {
        if (file_size == 0)
        {
            send(client_sock, buffer, 1, 0);
        }
        else
        {
            send(client_sock, buffer, file_size, 0);
        }
        if (file_size < sizeof(buffer))
        {
            break; // End of file
        }
    }

    close(file);
    sleep(5);
    snprintf(response, sizeof(response), "File %s uploaded successfully\n", filename);
    send(client_sock, response, strlen(response), 0);
}

void handle_rmfile(int client_sock, char *filename)
{
    char file_path[BUFFER_SIZE];

    snprintf(file_path, sizeof(file_path), "~/spdf/%s", filename);
    remove(file_path);
}

void handle_dtar(int client_sock, char *filetype)
{
    char tar_command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    FILE *tar_file;
    size_t file_size;

    if (strcmp(filetype, ".pdf") == 0)
    {
        snprintf(tar_command, sizeof(tar_command), "find ~/spdf -name '*.pdf' | tar -cvf pdffiles.tar -T -");
        system(tar_command);
        tar_file = fopen("pdffiles.tar", "rb");
    }
    else
    {
        char *msg = "Unsupported file type\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    if (!tar_file)
    {
        perror("Failed to open tar file");
        return;
    }

    // while ((file_size = fread(buffer, 1, BUFFER_SIZE, tar_file)) > 0)
    // {
    //     send(client_sock, buffer, file_size, 0);
    //     if (file_size < sizeof(buffer))
    //         {
    //             printf("End of file detected\n");
    //             break; // End of file
    //         }
    // }

    fclose(tar_file);
}
