/*******************************************************************************
* ** Author: Kyle Dixon
* ** Date: 6/5/2019-
* ** Descriptions: opt_enc_d daemon program (Block 4)
* ** 	works with opt_enc to create a cipher from the message (plaintext file) and return it 
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
    int messageLength;
    int portNumber;
    socklen_t msgLength;

    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    if (argc < 2){	//validate the argument number
        perror("Error: otp_enc_d arg number\n");
        exit(1);
    }

    portNumber = atoi(argv[1]);	//save the passed in port number and validate that it worked/ is an appropriate number
    if (portNumber < 0){
        perror("Error: otp_enc_d invalid port\n");
        exit(1);
    }

    memset(&serverAddress, '\0', sizeof(serverAddress));	//clear out the server address
    serverAddress.sin_family = AF_INET;	//create network capable socket
    serverAddress.sin_port = htons(portNumber);	//store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY;	//any address is allowed
    
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0){	//create the socket and validate
        perror("Error: opt_enc_d could not create socket\n");
        exit(1);
    }

    if (bind(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){	//connect the socket to the port and validate
        perror("Error: otp_enc_d unable to bind socket");
        exit(1);
    }

	listen(socketFD, 5);
    while (1) //set up to receive (accept a connection, blocking if one is not avaliable)
    {
    	msgLength = sizeof(clientAddress);	//get the length of the message from the server
        socketNewFD = accept(socketFD, (struct sockaddr *) &clientAddress, &msgLength);	//set up the new socket and validate
        if (socketNewFD < 0){
            perror("Error: opt_enc_d unable to accept connection\n");
            exit(1);
        }

        pid = fork();	//create a new porcess and validate
        if (pid < 0){
            perror("Error: opt_enc_d on fork\n");
            exit(1);
        }

        if (pid == 0)
        {
            memset(buffer1, 0, BUFFER_SIZE); //clear buffer1 for the incoming message

            messageLength = read(socketNewFD, buffer1, sizeof(buffer1)); //get the message and store in buffer1, validate the message
            if(strcmp(buffer1, "a") != 0)
            {
            	char out[] = "Error: invalid acknowledgment";
                write(socketNewFD, out, sizeof(out));
                exit(2);
			}
            if (messageLength < 0){
                printf("Error: otp_enc_d could not read plaintext");
                exit(1);
            }

            numSent = write(socketNewFD, "a", 1);	//send the acknowledgement back (a = acknowledgement)
            if (numSent < 0){
                perror("Error: otp_enc_d sending acknowledgement to client\n");
                exit(2);
            }

            memset(buffer2, 0, BUFFER_SIZE); //clear buffer2 for the incoming key
            keyLength = read(socketNewFD, buffer2, sizeof(buffer2)-1); //get the key and store in buffer2, validate the key
            if (keyLength < 0){
                perror("Error: otp_enc_d could not read key on port");
                exit(1);
            }

            for (i = 0; i < messageLength; i++) //validate the characters in the buffer (need to convert char to int with (int))
            {
                if ((int) buffer1[i] > 90 || ((int) buffer1[i] < 65 && (int) buffer1[i] != 32)){
                    perror("Error: otp_enc_d plaintext contains bad characters\n");
                    exit(1);
                }
            }
            for (i = 0; i < keyLength; i++) //validate the characters in the buffer (need to convert char to int with (int))
            {
                if ((int) buffer2[i] > 90 || ((int) buffer2[i] < 65 && (int) buffer2[i] != 32)){
                    perror("Error: otp_enc_d key contains bad characters\n");
                    exit(1);
                }
            }
            if (keyLength < messageLength){ //validate the lengths match
                perror("Error: otp_enc_d key length\n");
                exit(1);
            }

            for (i = 0; i < messageLength; i++)	//work through the message and save the converted message (cipher) in buffer3
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
                int cipher = (msg + key) % 27;	//combine with modular addition
                cipher = cipher + 64; //translate the ascii range back
                buffer3[i] = (char) cipher + 0; //convert ints back into chars
                if (buffer3[i] == '@') //change astericks back to spaces
                {
                    buffer3[i] = ' ';
                }
            }

            numSent = write(socketNewFD, buffer3, messageLength);	//send buffer3 (cipher) to otp_enc and validate
            if (numSent < messageLength){
                perror("Error: otp_enc_d writing to socket\n");
                exit(1);
            }

            close(socketNewFD);          //close sockets and exit
            exit(0);
        }
        else close(socketNewFD);
    }
    close(socketFD);
    return 0;
}
