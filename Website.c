#include <stdio.h>
#include <string.h>
#include <windows.h>


#pragma comment(lib,"ws2_32.lib") //Winsock Library

const char * header = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"\r\n";
const char * response = "HTTP/1.1 200 OK\r\n"
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

char * ConstructGETResponse(FILE * fp){
    rewind(fp);
    char * htmlBody = malloc(sizeof(char) * 1024);
    int htmlBodySize = 1024;
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
    }   
    printf("Message to send is: \"%s\"\n", htmlBody);
    char * htmlResponse = malloc(strlen(header)*sizeof(char) + htmlBodySize*sizeof(char) + 2*sizeof(char));
    htmlResponse[0] = '\0';
    strcat(htmlResponse, header);
    strcat(htmlResponse, htmlBody);
    free(htmlBody);
    fclose(fp);
    //printf("Message to send is: \"%s\"\n", htmlResponse);
    puts("Returning function.\n");
    return htmlResponse;

    
    
}

int HandleGETRequest(SOCKET Client, char input[]){
    FILE * fp;
    puts("Handling GET request.\n");
    char filepath [128];
    for(int i = 5; input[i] != ' ' && input[i] != '\n' && input[i] != '\0'; i++){
        filepath[i-5] = input[i];
        filepath[i-4] = '\0';
    }


    printf("FILEPATH IS: \"%s\"\n", filepath);
    fp = fopen(filepath, "r");


    if (fp == NULL){
        send(Client, response, strlen(response), 0);
    }
    else{
        char * htmlFile = ConstructGETResponse(fp);
        send(Client, htmlFile, strlen(htmlFile), 0);
        puts("Freeing htmlFile pointer.\n");
        free(htmlFile);
        puts("Freed htmlFile pointer.\n");
    }

    return 0;
    
}

DWORD WINAPI HandleConnection(LPVOID ClientPointer){
    SOCKET Client = * (SOCKET *)ClientPointer;
    char message[1024];
    int recv_length;
    puts("Beginning loop.\n");
    recv_length = recv(Client, message, 1024, 0);
    printf("Recv length is: %d\n", recv_length);
    if(recv_length == SOCKET_ERROR || recv_length == 0){
    puts("Client Connection broken.");
    closesocket(Client);
    }
    else{
        if(recv_length > 3){
            char requestType[] = {message[0], message[1], message[2], '\0'};//This only works for GET. I'm killing myself
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
    char message[1024];
    char input[24];

    if(WSAStartup((MAKEWORD(2, 2)), &wsa) != 0){
        printf("WSA Initialisation Error: %d\n", WSAGetLastError());
    }

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
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

    //The interesting stuff
    while(1){
        newSocket = accept(sock, (struct sockaddr *)&client, &c);
        puts("Connection established.\n\n\n");

        hThread = CreateThread(NULL, 0, HandleConnection, &newSocket, 0, NULL);
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
    
}
