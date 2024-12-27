#include <stdio.h>
#include <string.h>
#include <windows.h>


#pragma comment(lib,"ws2_32.lib") //Winsock Library

const char * header = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"\r\n";
const char * response = "HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html\r\n"
"\r\n" 
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<head>\r\n"
"  <title>Page Not Found</title>\r\n"
"</head>\r\n"
"<body>\r\n"
"  <h1>ERROR: 404!</h1>\r\n"
"  <p>Page not found.</p>\r\n"
"</body>\r\n"
"</html>\r\n";

struct httpRequest{
    char RequestType[4];
    char RequestedFilepath[128];

};


struct httpRequest * GetRequestHeaderInfo(char * Request){ 
    //This function goes through the http request line by line, and assigns values to the a httpRequest struct 
    //based on its findings, then returns the httpRequest info struct.
    struct httpRequest * HeaderInfo = malloc(sizeof(struct httpRequest));
    unsigned int i = 0;
    unsigned int count = 0;
    while(1){
        char * line = malloc(sizeof(char) * 128); //Allocating memory for the line buffer. Usually not enough. Maybe change that?
        unsigned int lineSize = 128;
        printf("Linesize = %d\n", lineSize);
        //printf("%s", Request);
        int pointer = 0;
        for (; Request[i] != ' ' && Request[i] != '\n' && Request[i] != '\0' && Request[i] != '\r'; i++){

            line[pointer] = Request[i];
            //"i" is the cursor location in the http request string.
            //"pointer" is the cursor location for the line buffer.
            //"i" should be reset only every http request (which is done by ensuring that "i"'s scope is this function),
            //while "pointer" should be reset every line.
            if(pointer >= lineSize - 2){
                realloc(line, lineSize + 128);
                puts("Newline buffer too small. Reallocating memory.");
                lineSize += 128;
                //If the pointer (the location where the newline sting membuff is being assigned) approaches the size of the 
                //memory buffer, increase the memory buffer.
            }
            printf("Assigning Request array number %d(I) to line array number %d, as char %c.\n", i, pointer, Request[i]);
            pointer ++;
        }
        i++;
        count++;
        line[pointer] = '\0';
        printf("Pointer = %d\nCount = %d\nI = %d\nLine = %s\n", pointer, count, i, line);

        switch (count){
        case 1:
            strcpy(HeaderInfo->RequestType, line);   //Switch case one should always be the request type
            break;
        
        case 2:
            for(int x = 1; line[x] != '\0'; x ++){ //This for loop shifts every digit over by one place to the left in the filepath line membuff.
            //This is necessary (or, janky), because fopen(/filepath) doesn't work for local files; it needs to be fopen(filepath).
                line[x-1] = line[x];
            }
            line[strlen(line)-1] = '\0';
            strcpy(HeaderInfo->RequestedFilepath, line); //Switch case two should always be the file path
            break;

        default:
            break;
        }
        if(Request[i] == '\0'){
            break;
        }

    }
    return HeaderInfo;

    
}


char * ConstructGETResponse(FILE * fp){
    rewind(fp);
    char * htmlBody = malloc(sizeof(char) * 1024); //Create buffer for the html file we want to send
    int htmlBodySize = 1024; //Keep track of the buffer size (we want to increase it, and we want to determine the send size (for the send() winapi function)).
    htmlBody[0] = '\0';
    char newLine[128];
    while(fgets(newLine, 128, fp) != NULL){
        if((strlen(htmlBody) + strlen(newLine)) > htmlBodySize){ //Increase buffer size to fit html file size. Probably.
            puts("Memory buffer too small. Increasing now.\n");
            htmlBody = realloc(htmlBody, htmlBodySize + 1024*sizeof(char));
            htmlBodySize += 1024;
            if(htmlBody == NULL){
                puts("Error reallocating memory.\n");
            }
        }
        strcat(htmlBody, newLine);
    }   //This while loop scans through every line of the html file. Then checks if adding the line to the html membuff will cause an overflow. If so, increase the membuff.
    //Then, it adds the line to the buffer storing the html file
    //If a line were long enough, it could increase the membuff and still overflow anyway (I aint fixing that.)
    printf("Message to send is: \"%s\"\n", htmlBody);
    char * htmlResponse = malloc(strlen(header)*sizeof(char) + htmlBodySize*sizeof(char) + 2*sizeof(char));
    htmlResponse[0] = '\0';//Creates a new variable rather than using the old one, because strcat() adds to the end, and we want to add something to the front (This is a hack)
    strcat(htmlResponse, header);
    strcat(htmlResponse, htmlBody); //
    free(htmlBody);//Free the membuff.
    fclose(fp);
    puts("Returning function.\n");
    return htmlResponse;

    
    
}

int HandleGETRequest(SOCKET Client, char input[]){
    FILE * fp;
    puts("Handling GET request.\n");
    //for(int i = 5; input[i] != ' ' && input[i] != '\n' && input[i] != '\0'; i++){
    //    filepath[i-5] = input[i];
    //    filepath[i-4] = '\0';
    //} This is hacky, old code to determine the filepath. (Also only would work on GET requests, or other requests with three leter names)
    struct httpRequest * HeaderInfo = GetRequestHeaderInfo(input);

    printf("FILEPATH IS: \"%s\"\n", HeaderInfo->RequestedFilepath);
    fp = fopen(HeaderInfo->RequestedFilepath, "r");


    if (fp == NULL){ //fp is NULL when the file cannot be opened.
        puts("File pointer is NULL (e.g, file does not exist).\n");
        send(Client, response, strlen(response), 0);
    }
    else{
        char * htmlFile = ConstructGETResponse(fp); //If it can be opened, construct a GET response, send it, and then free the membuff of the HTML file we scanned.
        send(Client, htmlFile, strlen(htmlFile), 0);
        puts("Freeing htmlFile pointer.\n");
        free(htmlFile);
        puts("Freed htmlFile pointer.\n");
    }
    free(HeaderInfo);

    return 0;
    
}

DWORD WINAPI HandleConnection(LPVOID ClientPointer){ //I fucking hate windows API
    SOCKET Client = * (SOCKET *)ClientPointer;
    char message[1024]; //Message is message from the client's browser
    int recv_length;
    puts("Beginning loop.\n");
    recv_length = recv(Client, message, 1024, 0); //Receive the message from the client and assign it to the "Message" membuff. Set recv_length (length of the received message).
    printf("Recv length is: %d\n", recv_length);
    if(recv_length == SOCKET_ERROR || recv_length == 0){ //Occurs when connection is closed or otherwise fails.
    puts("Client Connection broken.");
    closesocket(Client);
    }
    else{
        if(recv_length > 3){ //This is jank as shit. We have another way of determining the request type. Have to integrate it differently soon.
        //Probably have to determine values for the HTTP header here, and pass the pointer of it to "HandleGetRequest". She'll be right.
            char requestType[] = {message[0], message[1], message[2], '\0'};//This only works for GET. I'm killing myself. Hackiest shit I've ever written.
            printf("Request type: %s\n", requestType);
            if(strcmp(requestType, "GET") == 0){
                HandleGETRequest(Client, message);        
            }
            //send(Client, response, strlen(response), 0);
            printf("%s", message);
        }

        closesocket(Client);

    }
    return 0;
}

int main(){
    SOCKET sock, newSocket;
    WSADATA wsa;
    HANDLE hThread;
    struct sockaddr_in server, client;
    //char message[1024]; Why the fuck did I make message here as well? I don't remember. Maybe important? Maybe I was high?
    char input[24]; 

    if(WSAStartup((MAKEWORD(2, 2)), &wsa) != 0){
        printf("WSA Initialisation Error: %d\n", WSAGetLastError()); //Start WSA
    }

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){ //Bind socket. Return errors.
        printf("Error creating socket: %d\n", WSAGetLastError());
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(80);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR){
        printf("Binding error: %d\n", WSAGetLastError());
    }

    listen(sock, 5);
    int c = sizeof(struct sockaddr_in);

    //The interesting stuff //No it isn't.
    while(1){
        newSocket = accept(sock, (struct sockaddr *)&client, &c);
        puts("Connection established.\n\n\n");

        hThread = CreateThread(NULL, 0, HandleConnection, &newSocket, 0, NULL);
    }
    
    closesocket(sock); //This will never occur, as of now.
    WSACleanup();
    return 0;
    
}
