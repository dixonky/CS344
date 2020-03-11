/*******************************************************************************
* ** Author: Kyle Dixon
* ** Date: 6/5/2019-
* ** Descriptions: opt_enc program (Block 4)
* ** 	connects to otp_enc_d, and asks it to perform a one-time pad style encryption 
* ****************************************************************************/

#include <fcntl.h>   
#include <stdio.h>    
#include <stdlib.h>   
#include <unistd.h> 
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 70000


//
//
int main(int argc, char** argv)  //based off server.c code from class
{
    char buffer1[BUFFER_SIZE]; //used for message (plaintext file) and later translated message (cipher)
    char buffer2[BUFFER_SIZE]; //used for key to decode message
    char buffer3[1]; //used for opt_enc_d active communication
    int r;
    int i;
    int keyLength;
    int messageLength;
    int numReceived;
    int numSent;
    int portNumber;
    int socketFD;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 4){ //valide argument number
        perror("Error: otp_enc arg number\n");
        exit(1);
    }

    portNumber = atoi(argv[3]);	//save the passed in port number and validate that it worked/ is an appropriate number
    if (portNumber < 0){
        perror("Error: otp_enc invalid port\n");
        exit(1);
    }

    r = open(argv[1], O_RDONLY); //open the message file as read only and validate that it worked
    if (r < 0){
        perror("Error: otp_enc cannot open message file");
        exit(1);
    }
    messageLength = read(r, buffer1, sizeof(buffer1)-1); //get the length of the message and save the message into buffer1
    for (i = 0; i < messageLength - 1; i++) //validate the characters in the buffer (need to convert char to int with (int))
    {
        if ((int) buffer1[i] > 90 || ((int) buffer1[i] < 65 && (int) buffer1[i] != 32)){
            perror("Error: otp_enc bad characters\n");
            exit(1);
        }
    }
    close(r);
    
    r = open(argv[2], O_RDONLY); //open the key file and validate that it worked
    if (r < 0){
        perror("Error: otp_enc cannot open key file");
        exit(1);
    }
    
    keyLength = read(r, buffer2, sizeof(buffer2)-1); //get the key length
    for (i = 0; i < keyLength - 1; i++)
    {
        if ((int) buffer2[i] > 90 || ((int) buffer2[i] < 65 && (int) buffer2[i] != 32)){ 	//validate the characters in the buffer (need to convert char to int with (int))
            perror("Error: otp_enc bad characters\n");
            exit(1);
        }
    }
    close(r);

    if (keyLength < messageLength){  //check the key and message lengths match
        perror("Error: key length\n");
        exit(1);
    }

	//The files and arguments have been validated so it is time to connect to opt_enc_d
    socketFD = socket(AF_INET, SOCK_STREAM, 0);     //create the socket
    if (socketFD < 0){  //validate
        perror("Error: openning socket");
        exit(2);
    }
    server = gethostbyname("localhost");	//DNS lookup by name
    if (server == NULL) {  //validate
        perror("Error: connection to otp_enc_d\n");
        exit(1);
    }    
  
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);  //set up the server addresses, http://www.linuxhowtos.org/C_C++/socket.htm      
    serv_addr.sin_port = htons(portNumber); //connect to the server on the appropriate port
    
    memset(&serv_addr, '\0', sizeof(serv_addr)); //set up destination and source for copying
    if (connect(socketFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ 	//connect to otp_enc_d
        perror("Error: Hull Breach \n");
		exit(2);	//report hull breach failure
    }

    numSent = write(socketFD, buffer1, messageLength - 1);	//send message to otp_enc_d and validate that it worked
    if (numSent < messageLength - 1){
        perror("Error: could not send message to otp_enc_d");
        exit(2);
    }
    
    memset(buffer3, 0, 1); //clear buffer3 for the acknowledgement
    numReceived = read(socketFD, buffer3, 1);	//get acknowledgement from server and validate (authenitcation of connection)
    if (numReceived < 0){
      	perror("Error receiving acknowledgement from otp_enc_d\n");
      	exit(1);
    }

    numSent = write(socketFD, buffer2, keyLength - 1);	//send key to otp_enc_d and validate
    if (numSent < keyLength - 1){
        perror("Error: could not send key to otp_enc_d");
        exit(1);
    }
    
    memset(buffer1, 0, BUFFER_SIZE);  //clear buffer1 for the cipher
    do
    {
        numReceived = read(socketFD, buffer1, messageLength - 1);	//receive cipher from otp_enc_d, put in buffer 1, and validate
    }
    while (numReceived > 0);
    if (numReceived < 0){
       perror("Error receiving cipher text from otp_enc_d\n");
       exit(1);
    }
    
    for (i = 0; i < messageLength - 1; i++)
    {
        printf("%c", buffer1[i]);
    }
    printf("\n");
    close(socketFD);

    return 0;
}
