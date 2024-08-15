// stating all required libraries for the code here
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
#include <libgen.h>
// defining all necessary self defined macros which will be used through out the code
// this is the port where spdf will be running
#define PORT 8052
// this is the macros for buffer_size
#define BUFFER_SIZE 1024
// IP address which will be used for spdf server
#define ADDRESS "127.0.0.2"
const char *return_home_value();
void handle_client(int main_sock);
void handle_ufile(int main_sock, char *filename, char *dest_path, char *buffer);
void handle_dfile(int main_sock, char *filename);
void handle_rmfile(int main_sock, char *filename);
void handle_dtar(int main_sock, char *filetype);
void handle_display(int main_sock, char *pathname);
int main()
{
    // define the socket descriptors
    int server_sock, main_sock;
    // structure for server address and client address
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    // make socket connection
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    // set up server address
    // ipv4 address family
    server_addr.sin_family = AF_INET;
    // this converts the port number from host byte order to network byte order
    server_addr.sin_port = htons(PORT);
    // this function converts the IP address from text to binary form and stores it in sin_addr structure
    if (inet_pton(AF_INET, ADDRESS, &server_addr.sin_addr) <= 0)
    {
        perror("The given IP address is invalid");
        exit(EXIT_FAILURE);
    }
    // bind the server socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Failure of bind due to");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    // listen for the incoming message or commands from the client
    if (listen(server_sock, 3) < 0)
    {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    // print if socket sucessfully formed and now waiting for command
    printf("Stext server listening on port %d...\n", PORT);
    // go into infinite loop of accept to accept commands from client
    while ((main_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) >= 0)
    {
        printf("New client connected to Stext\n");
        // fork to handle the client commands
        if (fork() == 0)
        {
            close(server_sock);
            handle_client(main_sock); // using handle client do the needful actions
            close(main_sock);
            exit(0);
        }
        else
        {
            close(main_sock);
            wait(NULL); // wait in parent till child done
        }
    }
    if (main_sock < 0)
    {
        perror("Accept failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    close(server_sock);
    return 0;
}
// function split_path fucntion is to get filename from the path passeed to this function
// ignore all folders which are given in path and just save filename to target_file_name
void split_path(const char *full_path, char *folder_name, char *target_file_name)
{
    // Find the last occurrence of the '/' char
    const char *last_slash = strrchr(full_path, '/');
    // If a slash was found, separate the folder and file names
    if (last_slash != NULL)
    {
        // Copy the folder name (including the slash)
        size_t folder_len = last_slash - full_path + 1;
        strncpy(folder_name, full_path, folder_len);
        folder_name[folder_len] = '\0';
        // Copy the file name
        strcpy(target_file_name, last_slash + 1);
    }
    else
    {
        // If no slash is found, treat the entire path as a file name
        strcpy(folder_name, "");
        strcpy(target_file_name, full_path);
    }
}
// this function handles the client commands
void handle_client(int main_sock)
{
    char buffer[BUFFER_SIZE];
    // to store incoming data from client
    // buffers to store command and arguements from the user
    char command[BUFFER_SIZE];
    char arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];
    while (1)
    {
        // memset is used to clear the contents of buffer, command, arg1, and arg2
        // to ensure they are empty before each new read operation
        memset(buffer, 0, BUFFER_SIZE);
        memset(command, 0, BUFFER_SIZE);
        memset(arg1, 0, BUFFER_SIZE);
        memset(arg2, 0, BUFFER_SIZE);
        // Receive from client
        int recv_len = recv(main_sock, buffer, BUFFER_SIZE, 0);
        if (recv_len <= 0)
        {
            perror("recv failed"); // if failed print this
            break;
        }
        // Null-terminate the buffer
        buffer[recv_len] = '\0';
        sscanf(buffer, "%s %s %s", command, arg1, arg2);
        // compare the command and accordingly invoke the functions
        if (strcmp(command, "ufile") == 0)
        {
            handle_ufile(main_sock, arg1, arg2, buffer);
        }
        else if (strcmp(command, "dfile") == 0)
        {
            handle_dfile(main_sock, arg1);
        }
        else if (strcmp(command, "rmfile") == 0)
        {
            handle_rmfile(main_sock, arg1);
        }
        else if (strcmp(command, "dtar") == 0)
        {
            handle_dtar(main_sock, arg1);
        }
        else if (strcmp(command, "display") == 0)
        {
            handle_display(main_sock, arg1);
        }
        else
        {
            char *msg = "Invalid command\n";
            send(main_sock, msg, strlen(msg), 0);
        }
    }
}
// this function is a common fucntion to create a recursive directory if it's not there in the system
int create_directory_recursive(const char *path)
{
    // iinitializing string for temporary name of file
    char temp[256];
    // a null pointer initialization
    char *p = NULL;
    size_t len;
    // snprintf is to scan values and then add it to a single string
    // it works like concatenation of string
    // here file_name_temporary will be assigned with value of:  directory_to_be_created_path
    snprintf(temp, sizeof(temp), "%s", path);
    // get file_name_temporary length and store it to file_name_length
    len = strlen(temp);
    // set last char as null of file_name_temporary
    if (temp[len - 1] == '/')
    {
        temp[len - 1] = '\0';
    }
    // loop to create all non existed folder
    for (p = temp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(temp, 0755) != 0 && errno != EEXIST)
            {
                perror("mkdir");
                return -1; // on error return -1
            }
            *p = '/';
        }
    }
    // make last folder as well through this
    if (mkdir(temp, 0755) != 0 && errno != EEXIST)
    {
        perror("mkdir");
        return -1;
    }
    return 0; // on success return ZERO
}
// function manage_upload_file_to_server is to perform ufile
void handle_ufile(int main_sock, char *filename, char *dest_path, char *buffer)
{
    // initializing all required variables
    char file_path[BUFFER_SIZE];
    FILE *file;
    char response[BUFFER_SIZE];
    char destination_path[BUFFER_SIZE];
    char file_buffer[BUFFER_SIZE];
    int bytes_received;
    // folder_name tis to store all folders string
    char folder_name[1024];
    // base_filena is to store the end file name
    char base_filename[1024];
    // construct the folder path- directory_to_be_created_path -  on the server
    snprintf(destination_path, sizeof(destination_path), "%s/stext/%s", return_home_value(), dest_path);
    // Create the destination directory if it doesn't exist
    // if error then return
    if (create_directory_recursive(destination_path) != 0)
    {
        return;
    }
    // only fetch file name and ignore any directory path
    split_path(filename, folder_name, base_filename);
    // we are using return_home_value() function here to get HOME variable value to keep it dynamic
    // scan multiple strings to build document_location
    snprintf(file_path, sizeof(file_path), "%s/stext/%s/%s", return_home_value(), dest_path, base_filename);
    // Open the file at document_location for writig
    file = fopen(file_path, "wb");
    // if no document found then
    if (file == NULL)
    {
        // return this message to client using send()
        snprintf(response, sizeof(response), "Failed to open file %s for writing\n", file_path);
        send(main_sock, response, strlen(response), 0);
        return;
    }
    // Receive file data from the client
    printf("Receiving file: %s\n", file_path);
    // start reading this file from client and then wait for recieving this data
    // file will be read in chunks
    // recv() will ensure to wait for the send() call from client
    while ((bytes_received = recv(main_sock, file_buffer, sizeof(file_buffer), 0)) > 0)
    {
        printf("Received %d bytes\n", bytes_received);
        // fwrite to wrrite the content in new file name - file_string_storage
        fwrite(file_buffer, 1, bytes_received, file);
        if (bytes_received < sizeof(file_buffer))
        {
            printf("End of file detected\n");
            break; // End of file
        }
    }
    // on success scan this response and send it to client to state that new file at destinated location has been created and content has been added
    snprintf(response, sizeof(response), "File %s uploaded successfully\n", filename);
    send(main_sock, response, strlen(response), 0);
}
// function manage_download_file_to_serverfor dfile comamd
void handle_dfile(int main_sock, char *filename)
{
    // initializing all required variables
    // this is a buffer string to add content into it from any file
    char buffer[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    int file;
    int file_size;
    char response[BUFFER_SIZE];
    //// construct the folder path- document_location -  on the server
    snprintf(file_path, sizeof(file_path), "%s/stext/%s", return_home_value(), filename);
    printf("File to be uploaded from: %s\n", file_path);
    file = open(file_path, O_RDONLY);
    if (file < 0)
    {
        perror("Failed to open file");
        return;
    }
    // read file in chunks and then send the to client
    while ((file_size = read(file, buffer, sizeof(buffer))) >= 0)
    {
        if (file_size == 0)
        {
            // send it to client whoo is waiting to recieve this signal
            send(main_sock, buffer, 1, 0);
        }
        else
        {
            // send it to client whoo is waiting to recieve this signal
            send(main_sock, buffer, file_size, 0);
        }
        if (file_size < sizeof(buffer))
        {
            break; // End of file
        }
    }
    close(file);
    sleep(5);
    snprintf(response, sizeof(response), "File %s uploaded successfully\n", filename);
    send(main_sock, response, strlen(response), 0); // send the response to the client
}
// function manage_remove_file_from_server for rmfile comamd
void handle_rmfile(int main_sock, char *filename)
{
    char response[BUFFER_SIZE];
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/stext/%s", return_home_value(), filename);
    if (remove(file_path) < 0) // run remove() operation on it
    {
        // send() successfull message to client
        snprintf(response, sizeof(response), "File %s not found..\n", filename);
        send(main_sock, response, strlen(response), 0);
    }
    else
    {
        // send() successfull message to client
        snprintf(response, sizeof(response), "File %s deleted successfully.\n", filename);
        send(main_sock, response, strlen(response), 0);
    }
}
// this function handles the dtar command of the project
// this takes in the socket desc of the client and the type of file we want to tar
void handle_dtar(int main_sock, char *filetype)
{
    // this is a buffer string to add content into it from any file
    char tar_command[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    FILE *tar_file;
    size_t file_size;
    // this compares the agruement we have put to text and  if it is true then we enter the if statement
    if (strcmp(filetype, ".txt") == 0)
    {
        snprintf(tar_command, sizeof(tar_command), "find ~/stext -name '*.txt' | tar -cvf textfiles.tar -T -");
        system(tar_command);
        tar_file = fopen("textfiles.tar", "rb");
    }
    else
    {
        char *msg = "Unsupported file type\n";
        send(main_sock, msg, strlen(msg), 0);
        return;
    }
    // open tar file
    if (!tar_file)
    {
        perror("Failed to open tar file");
        return;
    }
    fclose(tar_file);
}
// handles the display commmand
void handle_display(int main_sock, char *pathname)
{
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    FILE *pipe;
    size_t file_size;
    // Construct the full path
    snprintf(command, sizeof(command), "%s/stext/%s", return_home_value(), pathname);
    // Check if the directory exists
    struct stat st;
    if (stat(command, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        // If the directory does not exist or is not a directory, return an empty response
        snprintf(buffer, sizeof(buffer), "DIRECTORY_NOT_FOUND\n"); // Send an empty string
        send(main_sock, buffer, strlen(buffer), 0);
        return;
    }
    // Construct the command to find .txt files in the specified directory
    snprintf(command, sizeof(command), "find %s/stext/%s -type f -name '*.txt'", return_home_value(), pathname);
    pipe = popen(command, "r"); // craete and open pipe
    if (!pipe)
    {
        perror("popen failed");
        return;
    }
    // Read the list of .txt files and send it to the client
    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = 0;                  // Remove newline char
        char *filename = basename(buffer);                  // Extract the file name
        snprintf(buffer, sizeof(buffer), "%s\n", filename); // Format the output with just the filename
        send(main_sock, buffer, strlen(buffer), 0);         // Send the filename to the client
    }
    pclose(pipe);
}
// returns the path
const char *return_home_value()
{
    return getenv("HOME");
}