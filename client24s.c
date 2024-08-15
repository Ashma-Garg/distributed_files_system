// stating all required libraries for the code here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

// if PATH_MAX is not found then self declare it
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
// defining all necessary self defined macros which will be used through out the code
// this is the port where client will be running
#define PORT 8051
// this is the macros for buffer_size
#define BUFFER_SIZE 1024
#define IP_ADDRESS "127.0.0.1"

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

// function deliver_command_to_server to send command to smain
empty_return_function deliver_command_to_server(number channel_for_client, constant character *instruction_from_user, constant character *parameter_1, constant character *parameter_2)
{
    // a buffer string for storing the values
    character string_storage[BUFFER_SIZE];
    // build string_storage string by concatinating instruction_from_user i.e. command, parameter_1 i.e arguemnt 1, parameter_2 i.e argueemnt 2
    snprintf(string_storage, sizeof(string_storage), "%s %s %s", instruction_from_user, parameter_1, parameter_2);

    // send this to server and if there is any error wehile sending thenn print error message
    if (send(channel_for_client, string_storage, strlen(string_storage), ZERO) == -1)
    {
        perror("deliver_command_to_server failed");
    }
}

// function deliver_file_to_server to send file from client to server
empty_return_function deliver_file_to_server(number channel_for_client, constant character *document_name)
{

    character string_storage[BUFFER_SIZE];
    // open file - document_name with read only access
    number document_a4 = open(document_name, O_RDONLY);
    // if error then print error message
    if (document_a4 < ZERO)
    {
        perror("open file");
        return;
    }
    // Check the size of the file
    off_t file_size = lseek(document_a4, ZERO, SEEK_END);
    // Rewind to the start of the file
    lseek(document_a4, ZERO, SEEK_SET);

    // if file has no content then state this message but still send it to smain
    if (file_size == ZERO)
    {
        show_on_cmd("File is empty: %s\n", document_name);
        // sending to smain
        send(channel_for_client, "", 1, ZERO);
    }
    // if there is content
    else
    {

        ssize_t bytes_read;
        // read the file and send that read content to smain
        while ((bytes_read = read(document_a4, string_storage, sizeof(string_storage))) > ZERO)
        {
            // send call to send it to smain
            // if there is any error while sending then break it
            if (send(channel_for_client, string_storage, bytes_read, ZERO) == -1)
            {
                perror("send file");
                break;
            }
        }
    }

    close(document_a4);
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

// Function to generate a unique document_name
empty_return_function get_unique_filename(constant character *folder_path, character *unique_filename)
{
    // to keep a count that how many files already exists with the same name
    number file_name_with_same_name_count = 1;
    character temp_filename[BUFFER_SIZE];
    // folder_name tis to store all folders string
    character folder_name[1024];
    // base_filena is to store the end file name
    character base_filename[1024];
    // base is to store filename before extension
    // like for file.c base_name will be file
    character base_name[BUFFER_SIZE];
    // extension will be .c
    character extension[BUFFER_SIZE];
    split_path(folder_path, folder_name, base_filename);
    // Split the base_filename into base_name and extension
    constant character *dot = strrchr(base_filename, '.');
    if (dot)
    {
        // Extract base name and extension
        size_t base_name_length = dot - base_filename;
        snprintf(base_name, base_name_length + 1, "%s", base_filename);
        snprintf(extension, BUFFER_SIZE, "%s", dot);
    }
    else
    {
        // No extension found
        snprintf(base_name, BUFFER_SIZE, "%s", base_filename);
        extension[ZERO] = '\0'; // No extension
    }

    // Combine the folder path and base document_name to form the initial full path
    snprintf(unique_filename, BUFFER_SIZE, "%s%s", folder_name, base_filename);

    // Check if file exists in the specified folder and generate a unique document_name if necessary
    while (access(unique_filename, F_OK) != -1)
    {
        snprintf(temp_filename, BUFFER_SIZE, "%s%s(%d)%s", folder_name, base_name, file_name_with_same_name_count++, extension);
        snprintf(unique_filename, BUFFER_SIZE, "%s", temp_filename);
    }
}

// this is to create the folders recursively
number create_directory_recursive(constant character *path)
{
    character temp[256];
    character *p = NULL;
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
            if (mkdir(temp, 0755) != ZERO && errno != EEXIST)
            {
                perror("mkdir");
                return -1;
            }
            *p = '/';
        }
    }

    return ZERO;
}

// function get_file_from_server is to get file content from smain
empty_return_function get_file_from_server(number channel_for_client, constant character *document_name)
{
    // to store the buffer string here in this variable
    character string_storage[BUFFER_SIZE];
    FILE *document_a4;
    ssize_t bytes_received;
    character unique_filename[BUFFER_SIZE];
    character file_path[BUFFER_SIZE];
    // to get the present working directory to fetch file
    character cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    character folder_name[1024];
    character base_filename[1024];

    split_path(document_name, folder_name, base_filename);
    snprintf(file_path, sizeof(file_path), "%s/%s", cwd, base_filename);

    // if there is already same name filename exists in the pwd then rename it and make it unique
    get_unique_filename(file_path, unique_filename);

    // open the file with write access
    document_a4 = fopen(unique_filename, "wb");
    // if it doesnt open it then through error message
    if (document_a4 == NULL)
    {
        perror("Failed to open file for writing");
        return;
    }

    // message on terminal that file is being recieved
    show_on_cmd("Receiving file: %s\n", unique_filename);
    // Receive the file data in chunks from smain and wait untill client gets it
    while ((bytes_received = recv(channel_for_client, string_storage, sizeof(string_storage), ZERO)) >= ZERO)
    {
        fwrite(string_storage, 1, bytes_received, document_a4);
        if (bytes_received < sizeof(string_storage))
        {
            break; // End of file
        }
    }

    // if none bytes recieved from the file then through error message
    if (bytes_received < ZERO)
    {
        perror("Failed to receive file");
    }
    // close the file
    fclose(document_a4);
    return;
}

// function manage_command_execution checks which command has been given by client
number manage_command_execution(number channel_for_client, constant character *instruction_from_user, constant character *parameter_1, constant character *parameter_2)
{
    // based on command implement functions
    // if ufile
    if (strcmp(instruction_from_user, "ufile") == ZERO)
    {
        number document_a4 = open(parameter_1, O_RDONLY);
        // if error then print error message
        if (document_a4 < ZERO)
        {
            perror("open file");
            return -1;
        }
        // go to deliver_command_to_server and send this command to smain to execute and take appropriate actions
        deliver_command_to_server(channel_for_client, instruction_from_user, parameter_1, parameter_2);
        // function to deliver file content to smain
        deliver_file_to_server(channel_for_client, parameter_1);
    }
    // if dfile
    else if (strcmp(instruction_from_user, "dfile") == ZERO)
    {
        // go to deliver_command_to_server and send this command to smain to execute and take appropriate actions
        deliver_command_to_server(channel_for_client, instruction_from_user, parameter_1, parameter_2);
        // function to receive file content from smain
        get_file_from_server(channel_for_client, parameter_1);
    }
    // if rmfile
    else if (strcmp(instruction_from_user, "rmfile") == ZERO)
    {
        // go to deliver_command_to_server and send this command to smain to execute and take appropriate actions
        deliver_command_to_server(channel_for_client, instruction_from_user, parameter_1, parameter_2);
    }
    // if dtar or display
    else if (strcmp(instruction_from_user, "dtar") == ZERO || strcmp(instruction_from_user, "display") == ZERO)
    {
        // go to deliver_command_to_server and send this command to smain to execute and take appropriate actions
        deliver_command_to_server(channel_for_client, instruction_from_user, parameter_1, parameter_2);
    }
    // if not valid command
    else
    {
        // print error message
        show_on_cmd("Unknown instruction_from_user: %s\n", instruction_from_user);
        return -1;
    }
}

// entry point of code
number main()
{
    // this is the socket initialization for client channel
    number channel_for_client;
    // socket addresses for server
    object sockaddr_in server_channel_address;
    // instruction_from_user is command or operation asked from client to perform
    // parameter_1 is arguement 1 of command if there is any
    // parameter_2 is arguement 2 of command if there is any
    character instruction_from_user[BUFFER_SIZE], parameter_1[BUFFER_SIZE], parameter_2[BUFFER_SIZE];
    // a temporary variable to store command and its parameters one by one
    character *temp_storage_for_command;
    // to store the inputed string from client in cmd
    character user_entered_command[BUFFER_SIZE];
    // make client socket here - TCP/IP socket
    //  AF_INET stands for "Address family internet". Here we are taking it from IPv4 addresses
    //  SOCK_STREAM is basically for providing reliable, two-way, connection-based byte streams. We use SOCKK_STREAM in TCP connection and SOCK_DGRAM for UDP
    //  ZERO is to choose protocol by default as per AF_INET and SOCK_STREAM
    //  here by default protocol would be TCP
    //  this "if" condition checks if there is any error while creating socket
    if ((channel_for_client = socket(AF_INET, SOCK_STREAM, ZERO)) < ZERO)
    {
        // if there is then print this message with error statement
        perror("Failure of client socket due to: ");
        // then exit from code
        exit(EXIT_FAILURE);
    }
    // this is used to specify what IP address family would be used for server socket
    server_channel_address.sin_family = AF_INET;
    // this is for assigning port to server
    // htons is used to convert - a host's byte order of 16-bit number (short) - from the to the network's byte order.
    server_channel_address.sin_port = htons(PORT);
    // inet_pton is used here to convert string IP address to binary form
    if (inet_pton(AF_INET, IP_ADDRESS, &server_channel_address.sin_addr) <= ZERO)
    {
        // if unsuccesfull to change then print error
        perror("Invalid address");
        // exit with failure coode
        exit(EXIT_FAILURE);
    }
    // Connect to server
    if (connect(channel_for_client, (object sockaddr *)&server_channel_address, sizeof(server_channel_address)) < ZERO)
    {
        // if not connected then print error and exit with failure
        perror("Failure of bind due to");
        exit(EXIT_FAILURE);
    }
    // infite loop to take command from client to perform through smain
    while (1)
    {
        // Get instruction_from_user from user
        show_on_cmd("Please give command: ");
        // if n o user_entered_command has been detected from client then print error message
        // and then continue from top of the loop using continue;
        if (fgets(user_entered_command, sizeof(user_entered_command), stdin) == NULL)
        {
            perror("fgets failed");
            continue;
        }
        // Remove newline character
        user_entered_command[strcspn(user_entered_command, "\n")] = ZERO;
        // Tokenize the user_entered_command means break it on basis of space and use command and parameter separately
        temp_storage_for_command = strtok(user_entered_command, " ");
        // if there is no command then pint error message to client
        if (temp_storage_for_command == NULL)
        {
            show_on_cmd("No command has been entered by you.\n");
            // then continue form top
            continue;
        }
        // keep building final command and store it into instruction_from_user
        strncpy(instruction_from_user, temp_storage_for_command, sizeof(instruction_from_user));
        // this will take parameter_1 or arguement 1
        temp_storage_for_command = strtok(NULL, " ");
        // if there is arguement 1 then
        if (temp_storage_for_command != NULL)
        {
            // add that arguement 1 to parameter_1
            strncpy(parameter_1, temp_storage_for_command, sizeof(parameter_1));
            // check for arguement 2
            temp_storage_for_command = strtok(NULL, " ");
            // if there is arguement 2
            if (temp_storage_for_command != NULL)
            {
                // store it to parameter_2
                strncpy(parameter_2, temp_storage_for_command, sizeof(parameter_2));
            }
            // otherwise keep it null
            else
            {
                parameter_2[ZERO] = '\0';
            }
        }
        // otherwise keep them null
        else
        {
            parameter_1[ZERO] = '\0';
            parameter_2[ZERO] = '\0';
        }
        // Send the instruction_from_user to manage_command_execution in order to check which function to be implemented based on command
        int ret = manage_command_execution(channel_for_client, instruction_from_user, parameter_1, parameter_2);
        if (ret == -1)
        {
            continue;
        }
        // This is to get final reponse from server or smain and then print it as indication that command has been executed to the end
        character reply_from_server[BUFFER_SIZE];
        // will wait untill it gets message from smain
        // if no message then in that case it won't move forward
        number len = recv(channel_for_client, reply_from_server, sizeof(reply_from_server) - 1, ZERO);
        // print that message on terminal
        if (len > ZERO)
        {
            reply_from_server[len] = '\0'; // Null-terminate the received data
            show_on_cmd("Server reply_from_server: %s\n", reply_from_server);
        }
        // state if server has been disconnected
        else if (len == ZERO)
        {
            show_on_cmd("Server disconnected.\n");
            break;
        }
        // if there was any issue with recv then state it through this message
        else
        {
            perror("recv");
        }
    }
    // completing all operations and then close the channel or socket
    close(channel_for_client);
    return ZERO;
}