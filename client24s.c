#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define PORT 8091
#define BUFFER_SIZE 1024

void send_command(int client_sock, const char *command, const char *arg1, const char *arg2)
{
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s %s %s", command, arg1, arg2);

    if (send(client_sock, buffer, strlen(buffer), 0) == -1)
    {
        perror("send_command failed");
    }
}

void send_file(int client_sock, const char *filename)
{
    char buffer[BUFFER_SIZE];
    int file = open(filename, O_RDONLY);
    if (file < 0)
    {
        perror("open file");
        return;
    }
    // Check the size of the file
    off_t file_size = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET); // Rewind to the start of the file

    if (file_size == 0)
    {
        printf("File is empty: %s\n", filename);
        send(client_sock, "", 1, 0);
    }
    else
    {
        ssize_t bytes_read;
        while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0)
        {
            if (send(client_sock, buffer, bytes_read, 0) == -1)
            {
                perror("send file");
                break;
            }
        }
    }

    close(file);
}

void split_path(const char *full_path, char *folder_name, char *file_name)
{
    // Find the last occurrence of the '/' character
    const char *last_slash = strrchr(full_path, '/');

    // If a slash was found, separate the folder and file names
    if (last_slash != NULL)
    {
        // Copy the folder name (including the slash)
        size_t folder_len = last_slash - full_path + 1;
        strncpy(folder_name, full_path, folder_len);
        folder_name[folder_len] = '\0';

        // Copy the file name
        strcpy(file_name, last_slash + 1);
    }
    else
    {
        // If no slash is found, treat the entire path as a file name
        strcpy(folder_name, "");
        strcpy(file_name, full_path);
    }
}

// Function to generate a unique filename
void get_unique_filename(const char *folder_path, char *unique_filename)
{
    int i = 1;
    char temp_filename[BUFFER_SIZE];
    char folder_name[1024];
    char base_filename[1024];

    // split_path(folder_path, folder_name, base_filename);

    // Combine the folder path and base filename to form the initial full path
    snprintf(unique_filename, BUFFER_SIZE, "%s", folder_path);

    // Check if file exists in the specified folder and generate a unique filename if necessary
    while (access(unique_filename, F_OK) != -1)
    {
        snprintf(temp_filename, BUFFER_SIZE, "%s%s(%d)", folder_path, base_filename, i++);
        snprintf(unique_filename, BUFFER_SIZE, "%s", temp_filename);
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

    return 0;
}

void receive_file(int client_sock, const char *filename)
{
    char buffer[BUFFER_SIZE];
    FILE *file;
    ssize_t bytes_received;
    char unique_filename[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    char folder_name[1024];
    char base_filename[1024];
    split_path(filename, folder_name, base_filename);
    snprintf(file_path, sizeof(file_path), "%s/%s", cwd, base_filename);

    get_unique_filename(file_path, unique_filename);

    file = fopen(unique_filename, "wb");
    if (file == NULL)
    {
        perror("Failed to open file for writing");
        return;
    }

    printf("Receiving file: %s\n", unique_filename);
    // Receive the file data in chunks
    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer), 0)) >= 0)
    {
        fwrite(buffer, 1, bytes_received, file);
        if (bytes_received < sizeof(buffer))
        {
            break; // End of file
        }
    }

    if (bytes_received < 0)
    {
        perror("Failed to receive file");
    }

    fclose(file);
    return;
}

void handle_command(int client_sock, const char *command, const char *arg1, const char *arg2)
{
    if (strcmp(command, "ufile") == 0)
    {
        send_command(client_sock, command, arg1, arg2);
        send_file(client_sock, arg1);
    }
    else if (strcmp(command, "dfile") == 0)
    {
        send_command(client_sock, command, arg1, arg2);
        receive_file(client_sock, arg1);
    }
    else if (strcmp(command, "rmfile") == 0) {
        send_command(client_sock, command, arg1, arg2);
    }
    else if (strcmp(command, "dtar") == 0 || strcmp(command, "display") == 0)
    {
        send_command(client_sock, command, arg1, arg2);
    }
    else
    {
        printf("Unknown command: %s\n", command);
    }
}

int main()
{
    int client_sock;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];
    char *token;
    char input[BUFFER_SIZE];

    // Create socket
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Get command from user
        printf("Enter command: ");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            perror("fgets failed");
            continue;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        // Tokenize the input
        token = strtok(input, " ");
        if (token == NULL)
        {
            printf("No command entered.\n");
            continue;
        }

        strncpy(command, token, sizeof(command));
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            strncpy(arg1, token, sizeof(arg1));
            token = strtok(NULL, " ");
            if (token != NULL)
            {
                strncpy(arg2, token, sizeof(arg2));
            }
            else
            {
                arg2[0] = '\0';
            }
        }
        else
        {
            arg1[0] = '\0';
            arg2[0] = '\0';
        }

        // Send the command
        handle_command(client_sock, command, arg1, arg2);
        // Receive and display response (optional)
        char response[BUFFER_SIZE];
        int len = recv(client_sock, response, sizeof(response) - 1, 0);
        if (len > 0)
        {
            response[len] = '\0'; // Null-terminate the received data
            printf("Server response: %s\n", response);
        }
        else if (len == 0)
        {
            printf("Server disconnected.\n");
            break;
        }
        else
        {
            perror("recv failed");
        }
    }

    close(client_sock);
    return 0;
}
