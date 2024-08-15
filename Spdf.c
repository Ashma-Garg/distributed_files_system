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
#define PORT 8094
// this is the macros for buffer_size
#define BUFFER_SIZE 1024
// IP address which will be used for spdf server
#define ADDRESS "127.0.0.3"
// redefining already defined data types in system
#define character char
#define constant const
#define number int
#define empty_return_function void
#define object struct
#define channel_length socklen_t
#define ZERO 0
#define show_on_cmd printf
// common varibales will be used through out the code
const char *UPLOAD_FILE = "ufile";
const char *DOWNLOAD_FILE = "dfile";
const char *REMOVE_FILE = "rmfile";
const char *GENERATE_TAR = "dtar";
const char *DISPLAY_LIST = "display";
constant character *return_home_value();
empty_return_function manage_client_interaction(number channel_for_client);
empty_return_function manage_upload_file_to_server(number channel_for_client, character *filename, character *dest_path, character *buffer);
empty_return_function manage_download_file_to_server(number channel_for_client, character *filename);
empty_return_function manage_remove_file_from_server(number channel_for_client, character *filename);
empty_return_function manage_add_tar_for_file_types_local(number channel_for_client, character *filetype);
empty_return_function manage_display_list_document_names_in_folder(number channel_for_client, character *pathname);
number main()
{
    // define the socket descriptors
    number channel_for_server, channel_for_client;
    // structure for server address and client address
    object sockaddr_in server_channel_address, client_channel_address;
    channel_length addr_len = sizeof(client_channel_address);
    // make socket connection
    if ((channel_for_server = socket(AF_INET, SOCK_STREAM, ZERO)) == ZERO)
    {
        perror("Faliure in pdf socket due to");
        exit(EXIT_FAILURE);
    }
    // set up server address
    // ipv4 address family
    server_channel_address.sin_family = AF_INET;
    // this converts the port number from host byte order to network byte order
    server_channel_address.sin_port = htons(PORT);
    // this function converts the IP address from text to binary form and stores it in sin_addr structure
    if (inet_pton(AF_INET, ADDRESS, &server_channel_address.sin_addr) <= ZERO)
    {
        perror("The given IP address is invalid");
        exit(EXIT_FAILURE);
    }
    // bind the server socket
    if (bind(channel_for_server, (object sockaddr *)&server_channel_address, sizeof(server_channel_address)) < ZERO)
    {
        perror("Failure of bind due to");
        close(channel_for_server);
        exit(EXIT_FAILURE);
    }
    // listen for the incoming message or commands from the client
    if (listen(channel_for_server, 3) < ZERO)
    {
        perror("Listen failed");
        close(channel_for_server);
        exit(EXIT_FAILURE);
    }
    // print if socket sucessfully formed and now waiting for command
    show_on_cmd("Spdf server listening on port %d...\n", PORT);
    // go into infinite loop of accept to accept commands from client
    while ((channel_for_client = accept(channel_for_server, (object sockaddr *)&client_channel_address, &addr_len)) >= ZERO)
    {
        show_on_cmd("New client connected to Spdf\n");
        // fork to handle the client commands
        if (fork() == ZERO)
        {
            close(channel_for_server);
            manage_client_interaction(channel_for_client); // using handle client do the needful actions
            close(channel_for_client);
            exit(ZERO);
        }
        else
        {
            close(channel_for_client);
            wait(NULL); // wait in parent till child done
        }
    }
    if (channel_for_client < ZERO)
    {
        perror("Accept failed");
        close(channel_for_server);
        exit(EXIT_FAILURE);
    }
    close(channel_for_server);
    return ZERO;
}
// function split_path fucntion is to get filename from the path passeed to this function
// ignore all folders which are given in path and just save filename to target_file_name
empty_return_function split_path(constant character *full_path, character *folder_name, character *target_file_name)
{
    // Find the last occurrence of the '/' character
    constant character *last_slash = strrchr(full_path, '/');
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
empty_return_function manage_client_interaction(number channel_for_client)
{
    character buffer[BUFFER_SIZE]; // to store incoming data from client
    // buffers to store command and arguements from the user
    character command[BUFFER_SIZE];
    character arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];
    while (1)
    {
        // memset is used to clear the contents of buffer, command, arg1, and arg2
        // to ensure they are empty before each new read operation
        memset(buffer, ZERO, BUFFER_SIZE);
        memset(command, ZERO, BUFFER_SIZE);
        memset(arg1, ZERO, BUFFER_SIZE);
        memset(arg2, ZERO, BUFFER_SIZE);
        // receive from client
        if (recv(channel_for_client, buffer, BUFFER_SIZE, ZERO) <= ZERO)
        {
            perror("recv failed"); // if failed print this
            break;
        }
        sscanf(buffer, "%s %s %s", command, arg1, arg2);
        // compare the command and accordingly invoke the functions
        if (strcmp(command, "ufile") == ZERO)
        {
            manage_upload_file_to_server(channel_for_client, arg1, arg2, buffer);
        }
        else if (strcmp(command, "dfile") == ZERO)
        {
            manage_download_file_to_server(channel_for_client, arg1);
        }
        else if (strcmp(command, "rmfile") == ZERO)
        {
            manage_remove_file_from_server(channel_for_client, arg1);
        }
        else if (strcmp(command, "dtar") == ZERO)
        {
            manage_add_tar_for_file_types_local(channel_for_client, arg1);
        }
        else if (strcmp(command, "display") == ZERO)
        {
            manage_display_list_document_names_in_folder(channel_for_client, arg1);
        }
        else
        {
            character *msg = "Invalid command\n";
            send(channel_for_client, msg, strlen(msg), ZERO);
        }
    }
}
// handles the display commmand
empty_return_function manage_display_list_document_names_in_folder(number channel_for_client, character *pathname)
{
    character buffer[BUFFER_SIZE];
    character command[BUFFER_SIZE];
    FILE *pipe;
    size_t file_size;
    // Construct the full path
    snprintf(command, sizeof(command), "%s/spdf/%s", return_home_value(), pathname);
    // Check if the directory exists
    object stat st;
    if (stat(command, &st) != ZERO || !S_ISDIR(st.st_mode))
    {
        // If the directory does not exist or is not a directory, return an empty response
        snprintf(buffer, sizeof(buffer), "DIRECTORY_NOT_FOUND\n"); // Send an empty string
        send(channel_for_client, buffer, strlen(buffer), ZERO);
        return;
    }
    // Construct the command to find .pdf files in the specified directory
    snprintf(command, sizeof(command), "find %s/spdf/%s -type f -name '*.pdf'", return_home_value(), pathname);
    pipe = popen(command, "r"); // craete and open pipe
    if (!pipe)
    {
        perror("popen failed");
        return;
    }
    // Read the list of .pdf files and send it to the client
    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = ZERO;                   // Remove newline character
        character *filename = basename(buffer);                 // Extract the file name
        snprintf(buffer, sizeof(buffer), "%s\n", filename);     // Format the output with just the filename
        send(channel_for_client, buffer, strlen(buffer), ZERO); // Send the filename to the client
    }
    pclose(pipe);
}
// this function is a common fucntion to create a recursive directory if it's not there in the system
number create_directory_recursive(constant character *path)
{
    // iinitializing string for temporary name of file
    character temp[256];
    // a null pointer initialization
    character *p = NULL;
    size_t len;
    // snprintf is to scan values and then add it to a single string
    // it works like concatenation of string
    // here file_name_temporary will be assigned with value of:  directory_to_be_created_path
    snprintf(temp, sizeof(temp), "%s", path);
    // get file_name_temporary length and store it to file_name_length
    len = strlen(temp);
    // set last character as null of file_name_temporary
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
            if (mkdir(temp, 0755) != ZERO && errno != EEXIST)
            {
                perror("mkdir");
                // on error return -1
                return -1;
            }
            *p = '/';
        }
    }
    // make last folder as well through this
    if (mkdir(temp, 0755) != ZERO && errno != EEXIST)
    {
        perror("mkdir");
        return -1;
    }
    return ZERO; // on success return ZERO
}
// function manage_upload_file_to_server is to perform ufile
empty_return_function manage_upload_file_to_server(number channel_for_client, character *filename, character *dest_path, character *buffer)
{
    // initializing all required variables
    character file_path[BUFFER_SIZE];
    FILE *file;
    size_t file_size;
    character response[BUFFER_SIZE];
    character destination_path[BUFFER_SIZE];
    character file_buffer[BUFFER_SIZE];
    number bytes_received;
    // folder_name tis to store all folders string
    character folder_name[1024];
    // base_filena is to store the end file name
    character base_filename[1024];
    // construct the folder path- directory_to_be_created_path -  on the server
    snprintf(destination_path, sizeof(destination_path), "%s/spdf/%s", return_home_value(), dest_path);
    // Create the destination directory if it doesn't exist
    // if error then return
    if (create_directory_recursive(destination_path) != ZERO)
    {
        return;
    }
    // only fetch file name and ignore any directory path
    split_path(filename, folder_name, base_filename);
    // we are using return_home_value() function here to get HOME variable value to keep it dynamic
    // scan multiple strings to build document_location
    snprintf(file_path, sizeof(file_path), "%s/spdf/%s/%s", return_home_value(), dest_path, base_filename);
    object stat st = {ZERO};
    // Open the file at document_location for writig
    file = fopen(file_path, "wb");
    // if no document found then
    if (file == NULL)
    {
        // return this message to client using send()
        snprintf(response, sizeof(response), "Failed to open file %s for writing\n", file_path);
        send(channel_for_client, response, strlen(response), ZERO);
        return;
    }
    // Receive file data from the client
    show_on_cmd("Receiving file: %s\n", file_path);
    // start reading this file from client and then wait for recieving this data
    // file will be read in chunks
    // recv() will ensure to wait for the send() call from client
    while ((bytes_received = recv(channel_for_client, file_buffer, sizeof(file_buffer), ZERO)) > ZERO)
    {
        // this will print the message on server that how many bytes will be written at the targetfile location
        show_on_cmd("Received %d bytes\n", bytes_received);
        // fwrite to wrrite the content in new file name - file_string_storage
        fwrite(file_buffer, 1, bytes_received, file);
        // if file ahs been read completely then break out of loop
        if (bytes_received < sizeof(file_buffer))
        {
            show_on_cmd("End of file detected\n");
            break; // End of file
        }
    }
    // on success scan this response and send it to client to state that new file at destinated location has been created and content has been added
    snprintf(response, sizeof(response), "File %s uploaded successfully\n", filename);
    send(channel_for_client, response, strlen(response), ZERO);
    fclose(file);
}
// function manage_download_file_to_serverfor dfile comamd
empty_return_function manage_download_file_to_server(number channel_for_client, character *filename)
{
    // initializing all required variables
    // this is a buffer string to add content into it from any file
    character buffer[BUFFER_SIZE];
    character file_path[BUFFER_SIZE];
    number file;
    number file_size;
    character response[BUFFER_SIZE];
    //// construct the folder path- document_location -  on the server
    snprintf(file_path, sizeof(file_path), "%s/spdf/%s", return_home_value(), filename);
    show_on_cmd("File to be uploaded from: %s\n", file_path);
    // open file with only read access
    file = open(file_path, O_RDONLY);
    // open file with only read access
    if (file < ZERO)
    {
        perror("Failed to open file");
        return;
    }
    // read file in chunks and then send the to client
    while ((file_size = read(file, buffer, sizeof(buffer))) >= ZERO)
    {
        if (file_size == ZERO)
        {
            // send it to client whoo is waiting to recieve this signal
            send(channel_for_client, buffer, 1, ZERO);
        }
        else
        {
            // send it to client whoo is waiting to recieve this signal
            send(channel_for_client, buffer, file_size, ZERO);
        }
        if (file_size < sizeof(buffer))
        {
            break; // End of file
        }
    }
    close(file);
    sleep(5);
    // build the string to send success message
    snprintf(response, sizeof(response), "File %s uploaded successfully\n", filename);
    // send the response to the client
    send(channel_for_client, response, strlen(response), ZERO);
}
// function manage_remove_file_from_server for rmfile comamd
empty_return_function manage_remove_file_from_server(number channel_for_client, character *filename)
{
    // initializing all required variables
    character file_path[BUFFER_SIZE];
    character response[BUFFER_SIZE];
    // build document_location out of HOME, docuemnt_name given by client
    snprintf(file_path, sizeof(file_path), "%s/spdf/%s", return_home_value(), filename);
    if (remove(file_path) < 0) // run remove() operation on it
    {
        // send() successfull message to client
        snprintf(response, sizeof(response), "File %s not found.\n", filename);
        send(channel_for_client, response, strlen(response), ZERO);
    }
    else
    {
        // send() successfull message to client
        snprintf(response, sizeof(response), "File %s deleted successfully.\n", filename);
        send(channel_for_client, response, strlen(response), ZERO);
    }
}
// this function handles the dtar command of the project
// this takes in the socket desc of the client and the type of file we want to tar
empty_return_function manage_add_tar_for_file_types_local(number channel_for_client, character *filetype)
{
    // this is a buffer string to add content into it from any file
    character tar_command[BUFFER_SIZE];
    character buffer[BUFFER_SIZE];
    FILE *tar_file;
    size_t file_size;
    // this compares the agruement we have put to pdf and  if it is true then we enter the if statement
    if (strcmp(filetype, ".pdf") == ZERO)
    {
        snprintf(tar_command, sizeof(tar_command), "find ~/spdf -name '*.pdf' | tar -cvf pdffiles.tar -T -");
        system(tar_command);
        tar_file = fopen("pdffiles.tar", "rb");
    }
    else
    {
        character *msg = "Unsupported file type\n";
        send(channel_for_client, msg, strlen(msg), ZERO);
        return;
    }
    // open tar file
    if (!tar_file)
    {
        perror("Failed to open tar file");
        return;
    }
    fclose(tar_file); // close tar file
}
// return the home path
constant character *return_home_value()
{
    return getenv("HOME");
}