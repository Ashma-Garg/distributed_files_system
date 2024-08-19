// stating all required libraries for the code here
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
#include <libgen.h>
// defining all necessary self defined macros which will be used through out the code
#define PORT 8053
#define string_storage_SIZE 1024
// text and pdf servers will be accessed through smain only
// these servers are hidden from client and client has no knowledge of it
// all server sockets will have different ports to run on like smain, stext and spdf
// PORT for pdf server to run on
#define SPDF_PORT 8094
// port for text server to run on
#define STEXT_PORT 8052
// kept their IP address different as well
// IP address which will be used for text server
#define TEXT_ADDRESS "127.0.0.2"
// IP address which will be used for pdf server
#define PDF_ADDRESS "127.0.0.3"
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
// these are the function declaration, also can be called a list of functions which will be used through this file
constant character *return_home_value();
empty_return_function manage_client_interaction(number channel_for_client);
empty_return_function manage_upload_file_to_server(number channel_for_client, character *document_name, character *target_location, character *string_storage);
empty_return_function manage_download_file_to_server(number channel_for_client, character *document_name, character *string_storage);
empty_return_function manage_remove_file_from_server(number channel_for_client, character *document_name, character *string_storage);
empty_return_function manage_add_tar_for_file_types_local(number channel_for_client, character *document_type);
empty_return_function manage_display_list_document_names_in_folder(number channel_for_client, character *pathname);
empty_return_function link_to_server(constant character *ip, number port, number *sock);
// entry point of code
number main()
{
    // initializing all sockets require in this process i.e for server it's channel_for_server and for client it's channel_for_client
    number channel_for_server, channel_for_client;
    // initializing channel or socket addresses for client and server
    object sockaddr_in server_channel_address, client_channel_address;
    // initializing variable to store length of client channel or socket address
    channel_length addr_len = sizeof(client_channel_address);
    // creating a socket for server
    // here socket() would create a socket and return a file_descriptor for this socket on success otherwise -1
    // AF_INET stands for "Address family internet". Here we are taking it from IPv4 addresses
    // SOCK_STREAM is basically for providing reliable, two-way, connection-based byte streams. We use SOCKK_STREAM in TCP connection and SOCK_DGRAM for UDP
    // ZERO is to choose protocol by default as per AF_INET and SOCK_STREAM
    // here by default protocol would be TCP
    // this if condition checks if there is any error while craeting socket
    if ((channel_for_server = socket(AF_INET, SOCK_STREAM, ZERO)) == ZERO)
    {
        // if there is then print this message with error statement
        perror("Failure of server socket due to");
        // then exit from code
        exit(EXIT_FAILURE);
    }
    // this is used to specify what IP address family would eb used for server socket
    server_channel_address.sin_family = AF_INET;
    // this is used to specify what IP address will be used for this server socket
    server_channel_address.sin_addr.s_addr = INADDR_ANY;
    // this is for assigning port to server
    // htons is used to convert - a host's byte order of 16-bit number (short) - from the to the network's byte order.
    server_channel_address.sin_port = htons(PORT);
    // this is to bind socket with IP address and PORT to know from where to listen for incoming connections or requests.
    if (bind(channel_for_server, (object sockaddr *)&server_channel_address, sizeof(server_channel_address)) < ZERO)
    {
        // if faced error print this message to user with actual error message
        perror("Failure of bind due to");
        // close the socket
        close(channel_for_server);
        // and exit fromt he code
        exit(EXIT_FAILURE);
    }
    // listen() prepares a sockets to accept incoming connection requests
    // here 3 specifies that how many pending request can be hold in a queue. By pending here, its the requests or connections which has not been accepted
    // server channel can have at most 3 pending connection requests here
    // if listen fails
    if (listen(channel_for_server, 3) < ZERO)
    {
        // the print this error message with appending actual error
        perror("Listen failed");
        // close server socket
        close(channel_for_server);
        // exit out of code
        exit(EXIT_FAILURE);
    }
    // if all went good then print this message to state to user that server is created successfully and listening to the desgnated port
    show_on_cmd("Main server channel listening at %d port!!!\n", PORT);
    // to accept conncetion requests after one anohter we have given accept() in a while loop
    // this while loop will check as long as socket is recieving connection requests and they are being accepted this loop will continue to go on
    // if we dont put accept in a never ending while loop then socket would listen limited connection requests
    // here accept() will accept the r=connection requests coming from client channel
    while ((channel_for_client = accept(channel_for_server, (object sockaddr *)&client_channel_address, &addr_len)) >= ZERO)
    {
        // print this message on server that  a client has been added and entered in this server from which this server will be accepting or recieving messages
        show_on_cmd("New client connected to Smain\n");
        // to keep child independent of parent we create a child here so that each client can be run separately
        // and doesn't interrupt with other clients' performance
        // as a single client will be in an infite loop each client needs a separate process to let other clients also use the parent process
        if (fork() == ZERO)
        {
            // if it's a child process for client execution then close server channel
            close(channel_for_server);
            // then go to manage_client_interaction function to see what type of operation or command needs to be executed
            // like ufile/ dfile/ rmfile/ dtar/ display
            manage_client_interaction(channel_for_client);
            // close client channel after completion of manage_client_interaction
            close(channel_for_client);
            // then exit without any error
            exit(ZERO);
        }
        else
        {
            // not using wait() here ensures that multiple clients will be able to interact with server
            // if parent then close client channel connaection
            close(channel_for_client);
        }
    }
    // if there is no client connection build by server then print error message to user that the connection through client was not accepted
    if (channel_for_client < ZERO)
    {
        // print this message to user with concated actual error message
        perror("Connection from client was not accepted!!! >_<");
        // close server connection as well
        close(channel_for_server);
        // exit the code with failure code
        exit(EXIT_FAILURE);
    }
    // close server connection if all clients has exited because of any error and there is nothing to do now
    close(channel_for_server);
    // return 0 to main
    return ZERO;
}
// function split_path fucntion is to get filename from the path passeed to this function
// ignore all folders which are given in path and just save filename to target_file_name
empty_return_function split_path(constant character *full_path, character *folder_name, character *target_file_name)
{
    // Find the last occurrence of the '/' character
    constant character *last_slash = strrchr(full_path, '/');
    // If a slash was found, separate the folder and file names

    //     /folder/folder_1/folder_2/folder_3/filename.c
    //     folder
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
// function manage_client_interaction started
empty_return_function manage_client_interaction(number channel_for_client)
{
    // this is a buffer string to add content into it from any file
    character string_storage[string_storage_SIZE];
    // this is command or operation asked from client to perform
    character instruction_from_user[string_storage_SIZE];
    // initializing variables for storing the arguments if any to command or operation given by user
    character parameter_1[string_storage_SIZE], parameter_2[string_storage_SIZE];
    // if true
    // endless loop
    while (1)
    {
        // memset sets a block of memory for a varibale to initialize the memory
        // initializing all non inialized varibales here which were arrays to allocate them memory
        memset(string_storage, ZERO, string_storage_SIZE);
        memset(instruction_from_user, ZERO, string_storage_SIZE);
        memset(parameter_1, ZERO, string_storage_SIZE);
        memset(parameter_2, ZERO, string_storage_SIZE);
        // recv() will wait until client sends any request by using send() function
        // recv() won't let the code move forward untill and unless it recieves any signal from client
        // here it's trying to recieve a request from channel_for_client where
        // request or command will be stores in string_storage varibale
        // command size or length will be stored in string_storage_SIZE variable
        // 0 at the end specifies that there are no special flags assigned here and no modification to the default implementation of recv() will take place
        number recv_len = recv(channel_for_client, string_storage, string_storage_SIZE - 1, ZERO);
        // if nothing can be recieved form cllient then end the loop and state error messages
        if (recv_len <= ZERO)
        {
            // in case of 0 commands
            if (recv_len == ZERO)
            {
                // say client has been disconnected
                show_on_cmd("This client has been disconnected or left!\n");
            }
            else
            {
                // else said receive failed
                perror("Receive (recv()): ");
            }
            // and then break out of infite loop
            break;
        }
        // if recv() runs successfully then
        // terminate the string_storage with null character
        string_storage[recv_len] = '\0';
        // we are using here sscanf to scan space separated string values in different variables as instruction_from_user i.e. command,
        // parameter_1 which is arg1 and parameter_2 which is arg2
        sscanf(string_storage, "%s %s %s", instruction_from_user, parameter_1, parameter_2);
        // these show_on_cmd statements are to check what commands are we getting from user or client
        show_on_cmd("Received instruction from client is: %s\n", instruction_from_user);
        show_on_cmd("Argument 1 provided by client: %s\n", parameter_1);
        show_on_cmd("Arguement 2 provided by client: %s\n", parameter_2);
        // ckeck which instruction_from_user or command has been asked from client
        // if its ufile then enter this if condition
        if (strcmp(instruction_from_user, "ufile") == ZERO)
        {
            // to to this function to perform specified fucntionality in if condition
            manage_upload_file_to_server(channel_for_client, parameter_1, parameter_2, string_storage);
        }
        // if its dfile then enter this if condition
        else if (strcmp(instruction_from_user, "dfile") == ZERO)
        {
            // to to this function to perform specified fucntionality in if condition
            manage_download_file_to_server(channel_for_client, parameter_1, string_storage);
        }
        // if its rmfile then enter this if condition
        else if (strcmp(instruction_from_user, "rmfile") == ZERO)
        {
            // to to this function to perform specified fucntionality in if condition
            manage_remove_file_from_server(channel_for_client, parameter_1, string_storage);
        }
        // if its dtar then enter this if condition
        else if (strcmp(instruction_from_user, "dtar") == ZERO)
        {
            // to to this function to perform specified fucntionality in if condition
            manage_add_tar_for_file_types_local(channel_for_client, parameter_1);
        }
        // if its display then enter this if condition
        else if (strcmp(instruction_from_user, "display") == ZERO)
        {
            // to to this function to perform specified fucntionality in if condition
            manage_display_list_document_names_in_folder(channel_for_client, parameter_1);
        }
        // if its invalid or out of scope command then enter this else condition
        else
        {
            // print this error message to client that this is not expected command
            character *msg = "Invalid instruction_from_user\n";
            // send this string to client channel or socket and it should be recieved by client in order to tell client about this articular error
            send(channel_for_client, msg, strlen(msg), ZERO);
        }
    }
}
// this function is a common fucntion to create a recursive directory if it's not there in the system
number create_directory_recursive(constant character *directory_to_be_created_path)
{
    // folder/folder_1/folder_2................
    // iinitializing string for temporary name of file
    character file_name_temporary[256];
    // a null pointer initialization
    character *temp_name_pointer = NULL;
    size_t file_name_length;
    // snprintf is to scan values and then add it to a single string
    // it works like concatenation of string
    // here file_name_temporary will be assigned with value of:  directory_to_be_created_path
    snprintf(file_name_temporary, sizeof(file_name_temporary), "%s", directory_to_be_created_path);
    // get file_name_temporary length and store it to file_name_length
    file_name_length = strlen(file_name_temporary);
    // set last char as null of file_name_temporary
    if (file_name_temporary[file_name_length - 1] == '/')
    {
        file_name_temporary[file_name_length - 1] = '\0';
    }
    // loop to create all non existed folder
    for (temp_name_pointer = file_name_temporary + 1; *temp_name_pointer; temp_name_pointer++)
    {
        if (*temp_name_pointer == '/')
        {
            *temp_name_pointer = '\0';
            if (mkdir(file_name_temporary, 0755) != ZERO && errno != EEXIST)
            {
                perror("mkdir");
                // on error return -1
                return -1;
            }
            *temp_name_pointer = '/';
        }
    }
    // make last folder as well through this
    if (mkdir(file_name_temporary, 0755) != ZERO && errno != EEXIST)
    {
        perror("mkdir");
        return -1;
    }
    // on success return 0
    return ZERO;
}
// function manage_upload_file_to_server is to perform ufile
empty_return_function manage_upload_file_to_server(number channel_for_client, character *document_name, character *target_location, character *string_storage)
{
    // initializing all required variables
    character document_location[string_storage_SIZE];
    character destination_path[string_storage_SIZE];
    FILE *document_a4;
    character reply_from_server[string_storage_SIZE];
    character file_string_storage[string_storage_SIZE];
    number document_byte_size;
    // folder_name tis to store all folders string
    character folder_name[1024];
    // base_filena is to store the end file name
    character base_filename[1024];
    // if the file type is c them communicate with smain
    if (strstr(document_name, ".c") != NULL)
    {
        // construct the folder path- directory_to_be_created_path -  on the server
        // ufile           pwd_folder/z.c      t_folder/t_folder_1
        // target_location - t_folder/t_folder_1
        snprintf(destination_path, sizeof(destination_path), "%s/smain/%s", return_home_value(), target_location);
        // Create the destination directory if it doesn't exist
        // if error then return
        if (create_directory_recursive(destination_path) != ZERO)
        {
            return;
        }
        // only fetch file name and ignore any directory path
        split_path(document_name, folder_name, base_filename);
        // we are using return_home_value() function here to get HOME variable value to keep it dynamic
        // scan multiple strings to build document_location
        snprintf(document_location, sizeof(document_location), "%s/smain/%s/%s", return_home_value(), target_location, base_filename);
        // document_location- home/smain/t_folder/t_folder_1/z.c
        document_a4 = fopen(document_location, "wb");
        // if no document found then
        if (document_a4 == NULL)
        {
            // return this message to client using send()
            snprintf(reply_from_server, sizeof(reply_from_server), "Failed to open file %s for writing\n", document_location);
            send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
            // then return
            return;
        }
        // Receive file data from the client
        show_on_cmd("Receiving file: %s\n", document_location);
        // start reading this file from client and then wait for recieving this data
        // file will be read in chunks
        // recv() will ensure to wait for the send() call from client
        while ((document_byte_size = recv(channel_for_client, file_string_storage, sizeof(file_string_storage), ZERO)) > ZERO)
        {
            // this will print the message on server that how many bytes will be written at the targetfile location
            show_on_cmd("Received %d bytes\n", document_byte_size);
            // fwrite to wrrite the content in new file name - file_string_storage
            fwrite(file_string_storage, 1, document_byte_size, document_a4);
            // if file ahs been read completely then break out of loop
            if (document_byte_size < sizeof(file_string_storage))
            {
                show_on_cmd("End of file detected\n");
                break; // End of file
            }
        }
        // on success scan this response and send it to client to state that new file at destinated location has been created and content has been added
        snprintf(reply_from_server, sizeof(reply_from_server), "File %s uploaded successfully\n", document_name);
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
    }
    // if file type is .txt then use stext
    else if (strstr(document_name, ".txt") != NULL)
    {
        // here we are initializing text socket
        number stext_sock;
        // this is a common function to build the conection
        link_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
        // after successfull connection send this command i.e. string_storage to stext server
        // if not able to send it to stext then
        if (send(stext_sock, string_storage, string_storage_SIZE, ZERO) == -1)
        {
            // build this error message and send it to client that there was an error
            snprintf(reply_from_server, sizeof(reply_from_server), "Not able to connect socket for text files", document_name);
            send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
        }
        // get file content from client and then use that content
        while ((document_byte_size = recv(channel_for_client, file_string_storage, sizeof(file_string_storage), ZERO)) > ZERO)
        {
            // to send it to the stext server
            send(stext_sock, file_string_storage, document_byte_size, ZERO);
            // break if file contentas been read completely
            if (document_byte_size < sizeof(file_string_storage))
            {
                break;
            }
        }
        // recieve end message from stext stating evrything went smoothly and then
        number len = recv(stext_sock, reply_from_server, sizeof(reply_from_server) - 1, ZERO);
        // sedn that message to client
        send(channel_for_client, reply_from_server, len, ZERO);
        // close stext connection
        close(stext_sock);
    }
    // if file type is .pdf then enter spdf
    else if (strstr(document_name, ".pdf") != NULL)
    {
        // here we are initializing pdf socket
        number spdf_sock;
        // this is a common function to build the conection
        link_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock);
        // after successfull connection send this command i.e. string_storage to stext server
        // if not able to send it to stext then
        if (send(spdf_sock, string_storage, string_storage_SIZE, ZERO) == -1)
        {
            // build this error message and send it to client that there was an error
            snprintf(reply_from_server, sizeof(reply_from_server), "Not able to connect socket for pdf files", document_name);
            send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
        }
        // get file content from client and then use that content
        while ((document_byte_size = recv(channel_for_client, file_string_storage, sizeof(file_string_storage), ZERO)) > ZERO)
        {
            // to send it to the spdf server
            send(spdf_sock, file_string_storage, document_byte_size, ZERO);
            // break if file contentas been read completely
            if (document_byte_size < sizeof(file_string_storage))
            {
                break;
            }
        }
        // recieve end message from spdf stating evrything went smoothly and then
        number len = recv(spdf_sock, reply_from_server, sizeof(reply_from_server) - 1, ZERO);
        // sedn that message to client
        send(channel_for_client, reply_from_server, len, ZERO);
        // close spdf connection
        close(spdf_sock);
    }
    // if any other file type has been given then state that it is not supported
    else
    {
        snprintf(reply_from_server, sizeof(reply_from_server), "File %s not supported for this process.\n", document_name);
        printf("File %s not supported for this process.\n", document_name);
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
    }
}
// function manage_download_file_to_serverfor dfile comamd
empty_return_function manage_download_file_to_server(number channel_for_client, character *document_name, character *initial_command)
{
    // initializing all required variables
    // this is a buffer string to add content into it from any file
    character string_storage[string_storage_SIZE];
    character document_location[string_storage_SIZE];
    number document_a4;
    number file_size;
    character reply_from_server[string_storage_SIZE];
    // if the file type is c them communicate with smain
    if (strstr(document_name, ".c") != NULL)
    {
        // construct the folder path- document_location -  on the server
        snprintf(document_location, sizeof(document_location), "%s/smain/%s", return_home_value(), document_name);
        show_on_cmd("File: %s\n", document_location);
        // open file with only read access
        document_a4 = open(document_location, O_RDONLY);
        // if no file has been found
        if (document_a4 < ZERO)
        {
            perror("Failed to open file");
            return;
        }
        // read file in chunks and then send the to client
        while ((file_size = read(document_a4, string_storage, sizeof(string_storage))) >= ZERO)
        {
            if (file_size == ZERO)
            {
                // send it to client whoo is waiting to recieve this signal
                send(channel_for_client, string_storage, 1, ZERO);
            }
            else
            {
                // send it to client whoo is waiting to recieve this signal
                send(channel_for_client, string_storage, file_size, ZERO);
            }
            // if file has been read completely then break the loop
            if (file_size < sizeof(string_storage))
            {
                break; // End of file
            }
        }
        // close the file after use
        close(document_a4);
        // sleep for 5 seconds
        sleep(5);
        // prepare the required success message for client
        snprintf(reply_from_server, sizeof(reply_from_server), "File %s downloaded successfully\n", document_name);
        // send the prepared message to client to know that file has been downloaded
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
    }
    // If file is .txt  fetch from Stext server
    else if (strstr(document_name, ".txt") != NULL)
    {
        // here we are initializing text socket
        number stext_sock;
        // this is a common function to build the conection
        link_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
        // after successfull connection send this command i.e. string_storage to stext server
        send(stext_sock, initial_command, strlen(initial_command), ZERO);
        // get file content from client and then use that content
        while ((file_size = recv(stext_sock, string_storage, string_storage_SIZE, ZERO)) >= ZERO)
        {
            // to send it to the stext server
            send(channel_for_client, string_storage, file_size, ZERO);
            // break if file contentas been read completely
            if (file_size < sizeof(string_storage))
            {
                break;
            }
        }
        // recieve end message from stext stating evrything went smoothly and then
        number len = recv(stext_sock, reply_from_server, sizeof(reply_from_server), ZERO);
        // send that message to client
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
        // close stext connection
        close(stext_sock);
    }
    // if file type is .pdf then enter spdf
    else if (strstr(document_name, ".pdf") != NULL)
    {
        // here we are initializing pdf socket
        number spdf_sock;
        // this is a common function to build the conection
        link_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock);
        // after successfull connection send this command i.e. string_storage to stext server
        send(spdf_sock, initial_command, strlen(initial_command), ZERO);
        // receive file content from client and then use that content
        while ((file_size = recv(spdf_sock, string_storage, string_storage_SIZE, ZERO)) >= ZERO)
        {
            // to send it to the spdf server
            send(channel_for_client, string_storage, file_size, ZERO);
            // break if file contentas been read completely
            if (file_size < sizeof(string_storage))
            {
                break;
            }
        }
        // recieve end message from spdf stating evrything went smoothly and then
        number len = recv(spdf_sock, reply_from_server, sizeof(reply_from_server), ZERO);
        // sedn that message to client
        send(channel_for_client, reply_from_server, sizeof(reply_from_server), ZERO);
        // close spdf connection
        close(spdf_sock);
    }
    // if any other file type has been given then state that it is not supported
    else
    {
        snprintf(reply_from_server, sizeof(reply_from_server), "File %s not supported for this process.\n", document_name);
        printf("File %s not supported for this process.\n", document_name);
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
    }
}
// function manage_remove_file_from_server for rmfile comamd
empty_return_function manage_remove_file_from_server(number channel_for_client, character *document_name, character *string_storage)
{
    // initializing all required variables
    character reply_from_server[string_storage_SIZE];
    // if the file type is c them communicate with smain
    if (strstr(document_name, ".c") != NULL)
    {
        // variable to store file location
        character document_location[string_storage_SIZE];
        // build document_location out of HOME, docuemnt_name given by client
        snprintf(document_location, sizeof(document_location), "%s/smain/%s", return_home_value(), document_name);
        // run remove() operation on it
        if (remove(document_location) < 0)
        {
            snprintf(reply_from_server, sizeof(reply_from_server), "File %s not found.\n", document_name);
            send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
        }
        else
        {
            // send() successfull message to client
            snprintf(reply_from_server, sizeof(reply_from_server), "File %s deleted successfully.\n", document_name);
            send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
        }
        // send() successfull message to client
        snprintf(reply_from_server, sizeof(reply_from_server), "File %s deleted successfully.\n", document_name);
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
    }
    // Request removal from Stext if the file is .txt
    if (strstr(document_name, ".txt") != NULL)
    {
        // here we are initializing text socket
        number stext_sock;
        // this is a common function to build the conection
        link_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
        // after successfull connection send this command i.e. string_storage to stext server
        send(stext_sock, string_storage, strlen(string_storage), ZERO);
        // recieve end message from stext stating evrything went smoothly and then
        number len = recv(stext_sock, reply_from_server, string_storage_SIZE, ZERO);
        // send that message to client
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
        // close stext connection
        close(stext_sock);
    }
    // if file type is .pdf then enter spdf
    else if (strstr(document_name, ".pdf") != NULL)
    {
        // here we are initializing pdf socket
        number spdf_sock;
        // this is a common function to build the conection
        link_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock);
        // after successfull connection send this command i.e. string_storage to stext server
        send(spdf_sock, string_storage, strlen(string_storage), ZERO);
        // recieve end message from spdf stating evrything went smoothly and then
        number len = recv(spdf_sock, reply_from_server, string_storage_SIZE, ZERO);
        // send that message to client
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
        // close spdf connection
        close(spdf_sock);
    }
    else {
        snprintf(reply_from_server, sizeof(reply_from_server), "File %s not supported for this process.\n", document_name);
        printf("File %s not supported for this process.\n", document_name);
        send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
    }
}
// this function handles the dtar command of the project
// this takes in the socket desc of the client and the type of file we want to tar
empty_return_function manage_add_tar_for_file_types_local(number channel_for_client, character *document_type)
{
    // this is a buffer string to add content into it from any file
    character string_storage[string_storage_SIZE]; // this stores the data that is sent or received via socket
    FILE *tar_file;                                // this is a pointer to the tar file that we will be making
    size_t file_size;                              // this var will store the size of the tar file.
    // this compares the agruement we have put to .c and  if it is true then we enter the if statement
    if (strcmp(document_type, ".c") == ZERO)
    {
        // Now we will handle .c files locally on Smain server
        // using this system call we will use find and find the c files in server and
        // then tar file is created which has all the c files
        system("find ~/smain -name '*.c' | tar -cvf cfiles.tar -T -");
        tar_file = fopen("cfiles.tar", "rb"); // this opens the tar file in read binary mode and tar file pointe
    }
    else if (strcmp(document_type, ".pdf") == ZERO) // this compares the agruement we have put to pdf and  if it is true then we enter the if statement
    {
        // Now we will handle pdf files locally on spdf server
        number spdf_sock;                                   // we are declaring socket descriptor
        link_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock); // this establishes connection to the pdf server
        // now we send the command to spdf server to create the tar file
        send(spdf_sock, "dtar .pdf", strlen("dtar .pdf"), ZERO);
        // now we close the socket descriptor as our work is done
        close(spdf_sock);
    }
    else if (strcmp(document_type, ".txt") == ZERO)
    {
        // Now we will handle text files locally on stext server
        // we are declaring socket descriptor
        number stext_sock;
        // this establishes connection to the text server
        link_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
        // Send instruction_from_user to Stext server to create the tar file
        send(stext_sock, "dtar .txt", strlen("dtar .txt"), ZERO);
        // close the socket descriptor
        close(stext_sock);
    }
    else
    { // if user enters any unsupported file type then this message is printed
        character *msg = "Unsupported file type\n";
        send(channel_for_client, msg, strlen(msg), ZERO);
        // the function then returns ending the execution
        return;
    }
    // declare a reply_from_server buffer to store the end reply_from_server to send to the client
    character reply_from_server[string_storage_SIZE];
    // this formats and stores the buffer in reply_from_server
    snprintf(reply_from_server, sizeof(reply_from_server), "File uploaded successfully\n");
    // now we send our reply_from_server to the client
    send(channel_for_client, reply_from_server, strlen(reply_from_server), ZERO);
}
// this function handles the display command
empty_return_function manage_display_list_document_names_in_folder(number channel_for_client, character *pathname)
{
    // this is a buffer string to add content into it from any file
    character string_storage[string_storage_SIZE];
    // Large string_storage to store the combined list
    character file_list[string_storage_SIZE * 10] = "";
    // pipe is pointing to a file object
    FILE *pipe;
    size_t file_size;             // declaring file size var
    number spdf_sock, stext_sock; // declaring socket descriptors
    // this will print the received pathname from client
    show_on_cmd("Display command : Received pathname: %s\n", pathname);
    // Step 1: Check if the directory exists locally for .c files
    character local_path[string_storage_SIZE];
    snprintf(local_path, sizeof(local_path), "%s/smain/%s", return_home_value(), pathname);
    object stat st;
    // check if directory exists

    // file_size, file_name, file_type
    if (stat(local_path, &st) == ZERO && S_ISDIR(st.st_mode))
    {
        // Directory exists, proceed to find .c files
        character instruction_from_user[string_storage_SIZE];
        // build the path and find the files in that folder/directory that are files
        snprintf(instruction_from_user, sizeof(instruction_from_user), "find %s/smain/%s -type f -name '*.c'", return_home_value(), pathname);
        // print that now we are executing the command
        show_on_cmd("Display command: Executing instruction_from_user: %s\n", instruction_from_user);
        // craetes the pipe and output of the find is written into this pipe
        pipe = popen(instruction_from_user, "r");
        if (pipe)
        {
            // read from the pipe using fgets and do the steps
            while (fgets(string_storage, sizeof(string_storage), pipe) != NULL)
            {
                string_storage[strcspn(string_storage, "\n")] = ZERO;               // Remove newline characteracter
                character *document_name = basename(string_storage);                // Extract the file name
                show_on_cmd("Display command: Found .c file: %s\n", document_name); // print the found files
                // append to the file_list the file names that we have found
                strncat(file_list, document_name, sizeof(file_list) - strlen(file_list) - 1);
                strncat(file_list, "\n", sizeof(file_list) - strlen(file_list) - 1);
            }
            pclose(pipe); // close pipe
        }
        else
        {
            perror("popen failed for smain");
        }
    }
    else
    { // if directory not found print this
        show_on_cmd("Display command: Directory %s not found in smain\n", local_path);
    }
    // Step 2: Communicate with Spdf server to get the list of .pdf files
    // connect to the pdf server
    link_to_server(PDF_ADDRESS, SPDF_PORT, &spdf_sock);
    // build the arguement string
    snprintf(string_storage, sizeof(string_storage), "display %s", pathname);
    // send the command to the spdf server
    send(spdf_sock, string_storage, strlen(string_storage), ZERO);
    // print on the server if send was successfully
    show_on_cmd("Display command: Sent display command to Spdf server\n");
    // var to keep count if we receive anything from spdf server
    // this is for the case if directory does not exist in the spdf server
    number received_anything_spdf = ZERO;
    // to receive from the server
    while ((file_size = recv(spdf_sock, string_storage, sizeof(string_storage) - 1, ZERO)) > ZERO)
    {
        // to make sure its null terminated
        string_storage[file_size] = '\0';
        // checks if directory empty
        if (strcmp(string_storage, "DIRECTORY_NOT_FOUND\n") == ZERO)
        {
            show_on_cmd("Display command: Directory not found on Spdf for %s\n", pathname);
            break; // break from the loop
        }
        // this means we have receieved files from the server
        received_anything_spdf = 1;
        // tokenize the string so that we get the file names properly
        character *line = strtok(string_storage, "\n");
        while (line != NULL)
        {
            character *document_name = basename(line);                                         // Extract the file name from the path
            show_on_cmd("Display command: Received .pdf file from Spdf: %s\n", document_name); // print the files receieved
            // append to the file_list the name pdf files
            strncat(file_list, document_name, sizeof(file_list) - strlen(file_list) - 1);
            strncat(file_list, "\n", sizeof(file_list) - strlen(file_list) - 1);
            line = strtok(NULL, "\n");
            // if we reach the end of file then break from loop
            if (file_size < sizeof(string_storage))
            {
                show_on_cmd("End of file detected\n");
                break; // End of file
            }
        }
        break;
    }
    close(spdf_sock); // close the socket
    if (received_anything_spdf == ZERO)
    {
        show_on_cmd("Display command: No .pdf files received from Spdf for %s\n", pathname);
    }
    // Step 3: Communicate with Stext server to get the list of .txt files
    // connect to the text server
    link_to_server(TEXT_ADDRESS, STEXT_PORT, &stext_sock);
    memset(string_storage, ZERO, string_storage_SIZE); 
    // build the arguement string
    snprintf(string_storage, sizeof(string_storage), "display %s", pathname);
    // send the command to the text server
    send(stext_sock, string_storage, strlen(string_storage), ZERO);
    // print on the server if send was successfully
    show_on_cmd("Display command: Sent display command to Stext server\n");
    // var to keep count if we receive anything from stext server
    // this is for the case if directory does not exist in the stext server
    number received_anything_stext = ZERO;
    // to receive from the server
    while ((file_size = recv(stext_sock, string_storage, sizeof(string_storage) - 1, ZERO)) > ZERO)
    {
        // to make sure its null terminated
        string_storage[file_size] = '\0';
        // checks if directory empty
        if (strcmp(string_storage, "DIRECTORY_NOT_FOUND\n") == ZERO)
        {
            show_on_cmd("Display command: Directory not found on Stext for %s\n", pathname);
            break; // break from the loop
        }
        // var to keep count if we receive anything from stext server
        // this is for the case if directory does not exist in the stext server
        received_anything_stext = 1;
        // tokenize the string so that we get the file names properly
        character *line = strtok(string_storage, "\n");
        while (line != NULL)
        {
            character *document_name = basename(line); // Extract the file name
            show_on_cmd("manage_display_list_document_names_in_folder: Received .txt file from Stext: %s\n", document_name);
            // append to the file_list the name text files
            strncat(file_list, document_name, sizeof(file_list) - strlen(file_list) - 1);
            strncat(file_list, "\n", sizeof(file_list) - strlen(file_list) - 1);
            line = strtok(NULL, "\n");
            // if we reach the end of file then break from loop
            if (file_size < sizeof(string_storage))
            {
                show_on_cmd("End of file detected\n");
                break; // End of file
            }
        }
        break;
    }
    close(stext_sock); // close socket
    if (received_anything_stext == ZERO)
    {
        show_on_cmd("Display command: No .txt files received from Stext for %s\n", pathname);
    }
    // Step 4: Send the combined list back to the client
    if (strlen(file_list) > 0)
    {
        show_on_cmd("Display command: Sending combined file list to client:\n%s", file_list);
        send(channel_for_client, file_list, strlen(file_list), ZERO);
    }
    else {
        send(channel_for_client, "Display command: No files were found.", strlen("Display command: No files were found."), ZERO);
    }
}
// this function establishes connection with the server
empty_return_function link_to_server(constant character *ip, number port, number *sock)
{
    // make socket structure
    object sockaddr_in server_channel_address;

    // make socket here - TCP/IP socket
    if ((*sock = socket(AF_INET, SOCK_STREAM, ZERO)) < ZERO)
    {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // set up server address
    server_channel_address.sin_family = AF_INET; // ipv4 address family
    // this converts the port number from host byte order to network byte order
    
    server_channel_address.sin_port = htons(port);
    // this function converts the IP address from text to binary form and stores it in sin_addr structure
    if (inet_pton(AF_INET, ip, &server_channel_address.sin_addr) <= ZERO)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // connect the socket here
    if (connect(*sock, (object sockaddr *)&server_channel_address, sizeof(server_channel_address)) < ZERO)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
}
// to get the home path for particular client
constant character *return_home_value()
{
    return getenv("HOME");
}