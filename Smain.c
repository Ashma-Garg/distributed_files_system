#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8091
#define BUFFER_SIZE 1024
#define SPDF_PORT 8088
#define STEXT_PORT 8089
#define TEXT_ADDRESS "127.0.0.2"
#define PDF_ADDRESS "127.0.0.3"

const char *return_home_value();
void handle_client(int client_sock);
void handle_ufile(int client_sock, char *filename, char *dest_path, char *buffer);
void handle_dfile(int client_sock, char *filename, char *buffer);
void handle_rmfile(int client_sock, char *filename, char *buffer);
void handle_dtar(int client_sock, char *filetype);
void handle_display(int client_sock, char *pathname);
void connect_to_server(const char *ip, int port, int *sock);

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 127.0.0.1
    server_addr.sin_port = htons(PORT);

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

    printf("Smain server listening on port %d...\n", PORT);

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) >= 0)
    {
        printf("New client connected to Smain\n");

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

        // Receive data from client
        int recv_len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (recv_len <= 0)
        {
            if (recv_len == 0)
            {
                printf("Client disconnected\n");
            }
            else
            {
                perror("recv failed");
            }
            break;
        }

        // Null-terminate the buffer
        buffer[recv_len] = '\0';

        // Parse command
        sscanf(buffer, "%s %s %s", command, arg1, arg2);

        printf("Received command: %s\n", command);
        printf("arg1: %s\n", arg1);
        printf("arg2: %s\n", arg2);

        // Handle command
        if (strcmp(command, "ufile") == 0)
        {
            handle_ufile(client_sock, arg1, arg2, buffer);
        }
        else if (strcmp(command, "dfile") == 0)
        {
            handle_dfile(client_sock, arg1, buffer);
        }
        else if (strcmp(command, "rmfile") == 0)
        {
            handle_rmfile(client_sock, arg1, buffer);
        }
        else if (strcmp(command, "dtar") == 0)
        {
            handle_dtar(client_sock, arg1);
        }
        else if (strcmp(command, "display") == 0)
        {
            handle_display(client_sock, arg1);
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
    char destination_path[BUFFER_SIZE];
    FILE *file;
    char response[BUFFER_SIZE];
    char file_buffer[BUFFER_SIZE];
    int bytes_received;

    if (strstr(filename, ".c") != NULL)
    {
        // Construct the file path on the server
        snprintf(destination_path, sizeof(destination_path), "%s/smain/%s", return_home_value(), dest_path);
        // Create the destination directory if it doesn't exist
        if (create_directory_recursive(destination_path) != 0)
        {
            return;
        }

        snprintf(file_path, sizeof(file_path), "%s/smain/%s/%s", return_home_value(), dest_path, filename);
        // Open the file for writing
        file = fopen(file_path, "wb");
        if (file == NULL)
        {
            snprintf(response, sizeof(response), "Failed to open file %s for writing\n", file_path);
            send(client_sock, response, strlen(response), 0);
            return;
        }

        // Receive file data from the client
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
    }
    else if (strstr(filename, ".txt") != NULL)
    {
        int stext_sock;
        connect_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
        if (send(stext_sock, buffer, BUFFER_SIZE, 0) == -1)
        {
            snprintf(response, sizeof(response), "Not able to connect socket for text files", filename);
            send(client_sock, response, strlen(response), 0);
        }

        while ((bytes_received = recv(client_sock, file_buffer, sizeof(file_buffer), 0)) > 0)
        {
            send(stext_sock, file_buffer, bytes_received, 0);
            if (bytes_received < sizeof(file_buffer))
            {
                break; // End of file
            }
        }
        int len = recv(stext_sock, response, sizeof(response) - 1, 0);
        send(client_sock, response, len, 0);
        close(stext_sock);
    }
    else if (strstr(filename, ".pdf") != NULL)
    {
        int spdf_sock;

        connect_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock);
        if (send(spdf_sock, buffer, BUFFER_SIZE, 0) == -1)
        {
            snprintf(response, sizeof(response), "Not able to connect socket for pdf files", filename);
            send(client_sock, response, strlen(response), 0);
        }

        while ((bytes_received = recv(client_sock, file_buffer, sizeof(file_buffer), 0)) > 0)
        {
            send(spdf_sock, file_buffer, bytes_received, 0);
            if (bytes_received < sizeof(file_buffer))
            {
                break; // End of file
            }
        }

        int len = recv(spdf_sock, response, sizeof(response) - 1, 0);
        send(client_sock, response, len, 0);
        close(spdf_sock);
    }
    else
    {
        snprintf(response, sizeof(response), "File %s not supported for this process.\n", filename);
        send(client_sock, response, strlen(response), 0);
    }
}

void handle_dfile(int client_sock, char *filename, char *initial_command)
{
    char buffer[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    int file;
    int file_size;
    char response[BUFFER_SIZE];

    if (strstr(filename, ".c") != NULL)
    {
        snprintf(file_path, sizeof(file_path), "%s/smain/%s", return_home_value(), filename);
        printf("File: %s\n", file_path);

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
    // If file is .txt or .pdf, fetch from Spdf or Stext server
    else if (strstr(filename, ".txt") != NULL)
    {
        int stext_sock;
        connect_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
        send(stext_sock, initial_command, strlen(initial_command), 0);
        while ((file_size = recv(stext_sock, buffer, BUFFER_SIZE, 0)) >= 0)
        {
            send(client_sock, buffer, file_size, 0);
            if (file_size < sizeof(buffer))
            {
                break; // End of file
            }
        }
        int len = recv(stext_sock, response, sizeof(response), 0);
        send(client_sock, response, strlen(response), 0);
        close(stext_sock);
    }
    else if (strstr(filename, ".pdf") != NULL)
    {
        int spdf_sock;
        connect_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock);
        send(spdf_sock, initial_command, strlen(initial_command), 0);
        while ((file_size = recv(spdf_sock, buffer, BUFFER_SIZE, 0)) >= 0)
        {
            send(client_sock, buffer, file_size, 0);
            if (file_size < sizeof(buffer))
            {
                break; // End of file
            }
        }

        int len = recv(spdf_sock, response, sizeof(response), 0);
        send(client_sock, response, sizeof(response), 0);
        close(spdf_sock);
    }
    else
    {
        snprintf(response, sizeof(response), "File %s not supported for this process.\n", filename);
        send(client_sock, response, strlen(response), 0);
    }
}

void handle_rmfile(int client_sock, char *filename, char *buffer)
{
    char response[BUFFER_SIZE];
    if (strstr(filename, ".c") != NULL)
    {
        char file_path[BUFFER_SIZE];

        snprintf(file_path, sizeof(file_path), "%s/smain/%s", return_home_value(), filename);
        remove(file_path);
        snprintf(response, sizeof(response), "File %s deleted successfully.\n", filename);
        send(client_sock, response, strlen(response), 0);
    }
    // Request removal from Spdf or Stext if the file is .pdf or .txt
    if (strstr(filename, ".txt") != NULL)
    {
        int stext_sock;
        connect_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
        send(stext_sock, buffer, strlen(buffer), 0);
        int len = recv(stext_sock, response, BUFFER_SIZE, 0);
        send(client_sock, response, strlen(response), 0);
        close(stext_sock);
    }
    else if (strstr(filename, ".pdf") != NULL)
    {
        int spdf_sock;
        connect_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock);
        send(spdf_sock, buffer, strlen(buffer), 0);
        int len = recv(spdf_sock, response, BUFFER_SIZE, 0);
        send(client_sock, response, strlen(response), 0);
        close(spdf_sock);
    }
}

void handle_dtar(int client_sock, char *filetype)
{
    char tar_command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    FILE *tar_file;
    size_t file_size;

    if (strcmp(filetype, ".c") == 0)
    {
        snprintf(tar_command, sizeof(tar_command), "tar -cvf cfiles.tar ~/smain/*.c");
        system(tar_command);
        tar_file = fopen("cfiles.tar", "rb");
    }
    else if (strcmp(filetype, ".pdf") == 0)
    {
        snprintf(tar_command, sizeof(tar_command), "tar -cvf pdfs.tar ~/spdf/*.pdf");
        system(tar_command);
        tar_file = fopen("pdfs.tar", "rb");
    }
    else if (strcmp(filetype, ".txt") == 0)
    {
        snprintf(tar_command, sizeof(tar_command), "tar -cvf txts.tar ~/stext/*.txt");
        system(tar_command);
        tar_file = fopen("txts.tar", "rb");
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

    while ((file_size = fread(buffer, 1, BUFFER_SIZE, tar_file)) > 0)
    {
        send(client_sock, buffer, file_size, 0);
    }

    fclose(tar_file);
}

void handle_display(int client_sock, char *pathname)
{
    char command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    FILE *pipe;
    size_t file_size;

    snprintf(command, sizeof(command), "find ~/smain/%s -type f \( -name '*.c' -o -name '*.pdf' -o -name '*.txt' \)", pathname);
    pipe = popen(command, "r");
    if (!pipe)
    {
        perror("popen failed");
        return;
    }

    while ((file_size = fread(buffer, 1, BUFFER_SIZE, pipe)) > 0)
    {
        send(client_sock, buffer, file_size, 0);
    }

    pclose(pipe);
}

void connect_to_server(const char *ip, int port, int *sock)
{
    struct sockaddr_in server_addr;

    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(*sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
}

const char *return_home_value() {
    return getenv("HOME");
}