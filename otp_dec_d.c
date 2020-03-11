/*******************************************************************************
* ** Author: Kyle Dixon
* ** Date: 6/5/2019-
* ** Descriptions: opt_dec_d daemon program (Block 4)
* ** 	works with opt_dec to decode the cipher (plaintext file) and return it as the translated message
* ****************************************************************************/
#include <stdio.h>     
#include <stdlib.h>   
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>     
#include <netinet/in.h>

#define BUFFER_SIZE 70000

//
//
void error(const char *msg) //http://www.linuxhowtos.org/data/6/server.c
{
    perror(msg);
    exit(1);
}

void error2(const char *msg) //http://www.linuxhowtos.org/data/6/server.c
{
    perror(msg);
    exit(2);
}


//
//
int main(int argc, char** argv) //based off client.c code from class
{
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    char buffer3[BUFFER_SIZE];
    int socketFD;
    int socketNewFD;
    int i;
    int keyLength;
    int numSent;
    pid_t pid;
    int cypherLength;
    int portNumber;
    socklen_t msgLength;

    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    if (argc < 2){	//validate the argument number
        error("Error: otp_dec_d arg number\n");
    }

    portNumber = atoi(argv[1]);	//save the passed in port number and validate that it worked/ is an appropriate number
    if (portNumber < 0){
        error2("Error: otp_dec_d invalid port\n");
    }

    memset(&serverAddress, '\0', sizeof(serverAddress));	//clear out the server address
    serverAddress.sin_family = AF_INET;	//create network capable socket
    serverAddress.sin_port = htons(portNumber);	//store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY;	//any address is allowed
    
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){	//create the socket and validate
        error("Error: opt_dec_d could not create socket\n");
    }

    if (bind(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){	//connect the socket to the port and validate
        printf("Error: otp_dec_d unable to bind socket to port %d\n", portNumber);
        exit(2);
    }

    if (listen(socketFD, 5) == -1){	//flip the socket on with 5 possible connections and validate
        printf("Error: otp_dec_d unable to listen on port %d\n", portNumber);
        exit(2);
    }

    while (1) //set up to receive (accept a connection, blocking if one is not avaliable)
    {
    	msgLength = sizeof(clientAddress);	//get the length of the cipher from the server
        socketNewFD = accept(socketFD, (struct sockaddr *) &clientAddress, &msgLength);	//set up the new socket and validate
        if (socketNewFD < 0){
            error("Error: opt_dec_d unable to accept connection\n");
        }

        pid = fork();	//create a new porcess and validate
        if (pid < 0){
            error("Error: opt_dec_c on fork\n");
        }

        if (pid == 0)
        {
            memset(buffer1, 0, BUFFER_SIZE); //clear buffer1 for the incoming cipher
            cypherLength = read(socketNewFD, buffer1, BUFFER_SIZE); //get the cipher and store in buffer1, validate the cipher
            if (cypherLength < 0){
                printf("Error: otp_dec_d could not read plaintext on port %d\n", portNumber);
                exit(2);
            }

            numSent = write(socketNewFD, "!", 1);	//send the acknowledgement back (! = acknowledgement)
            if (numSent < 0){
                error2("Error: otp_dec_d sending acknowledgement to client\n");
            }

            memset(buffer2, 0, BUFFER_SIZE); //clear buffer2 for the incoming key
            keyLength = read(socketNewFD, buffer2, BUFFER_SIZE); //get the key and store in buffer2, validate the key
            if (keyLength < 0){
                printf("Error: otp_dec_d could not read key on port %d\n", portNumber);
                exit(2);
            }

            for (i = 0; i < cypherLength; i++) //validate the characters in the buffer (need to convert char to int with (int))
            {
                if ((int) buffer1[i] > 90 || ((int) buffer1[i] < 65 && (int) buffer1[i] != 32)){
                    error("Error: otp_dec_d plaintext contains bad characters\n");
                }
            }
            for (i = 0; i < keyLength; i++) //validate the characters in the buffer (need to convert char to int with (int))
            {
                if ((int) buffer2[i] > 90 || ((int) buffer2[i] < 65 && (int) buffer2[i] != 32)){
                    error("Error: otp_dec_d key contains bad characters\n");
                }
            }
            if (keyLength < cypherLength){ //validate the lengths match
                error("Error: otp_dec_d key length\n");
            }

            for (i = 0; i < cypherLength; i++)	//work through the message and save the converted message (cipher) in buffer3
            {
                if (buffer1[i] == ' ') //change spaces to asterisks
                {
                    buffer1[i] = '@';
                }
                if (buffer2[i] == ' ')
                {
                    buffer2[i] = '@';
                }
                int msg = (int) buffer1[i];	//holders for the active characters (convert to ints)
                int key = (int) buffer2[i];
                msg = msg - 64;	//translate the ascii range into 0 - 26
                key = key - 64;
                int solved = msg - key;	//combine with modular subtraction
                if (solved < 0) 
                {
                    solved = solved + 27;
                }
                solved = solved + 64; //translate the ascii range back
                buffer3[i] = (char) solved + 0; //convert ints back into chars
                if (buffer3[i] == '@') //change astericks back to spaces
                {
                    buffer3[i] = ' ';
                }
            }

            numSent = write(socketNewFD, buffer3, cypherLength);	//send buffer3 (cipher) to otp_dec and validate
            if (numSent < cypherLength){
                error2("Error: otp_dec_d writing to socket\n");
            }

            close(socketNewFD);          //close sockets and exit
            close(socketFD);
            exit(0);
        }

        else close(socketNewFD);
    }
    return 0;
}
