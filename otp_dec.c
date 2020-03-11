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
int main(int argc, char** argv)  //based off server.c code from class
{
    char buffer1[BUFFER_SIZE]; //used for cipher (plaintext file) and later translated original message (cipher)
    char buffer2[BUFFER_SIZE]; //used for key to decode message
    char buffer3[1]; //used for opt_dec_d communication
    int r;
    int i;
    int keyLength;
    int cypherLength;
    int numReceived;
    int numSent;
    int portNumber;
    int socketFD;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 4){ //valide argument number
        error("Error: otp_dec arg number\n");
    }

    portNumber = atoi(argv[3]);	//save the passed in port number and validate that it worked/ is an appropriate number
    if (portNumber < 0){
        error2("Error: otp_dec invalid port\n");
    }

    r = open(argv[1], O_RDONLY); //open the cipher file as read only and validate that it worked
    if (r < 0){
        printf("Error: otp_dec cannot open message file %s\n", argv[1]);
        exit(1);
    }
    
    cypherLength = read(r, buffer1, BUFFER_SIZE); //get the length of the cipher and save it into buffer1
    for (i = 0; i < cypherLength - 1; i++) //validate the characters in the buffer (need to convert char to int with (int))
    {
        if ((int) buffer1[i] > 90 || ((int) buffer1[i] < 65 && (int) buffer1[i] != 32)){
            error("Error: otp_dec bad characters\n");
        }
    }
    close(r);
    
    r = open(argv[2], O_RDONLY); //open the key file and validate that it worked
    if (r < 0){
        printf("Error: otp_dec cannot open key file %s\n", argv[2]);
        exit(1);
    }
    keyLength = read(r, buffer2, BUFFER_SIZE); //get the key length
    for (i = 0; i < keyLength - 1; i++)
    {
        if ((int) buffer2[i] > 90 || ((int) buffer2[i] < 65 && (int) buffer2[i] != 32)){ 	//validate the characters in the buffer (need to convert char to int with (int))
            error("Error: otp_dec bad characters\n");
        }
    }
    close(r);

    if (keyLength < cypherLength){  //check the key and cipher lengths match
        printf("Error: key '%s' length\n", argv[2]);
        exit(1);
    }

	//The files and arguments have been validated so it is time to connect to opt_dec_d
    socketFD = socket(AF_INET, SOCK_STREAM, 0);     //create the socket
    if (socketFD < 0){  //validate
        printf("Error: openning socket %d\n", portNumber);
        exit(2);
    }
    server = gethostbyname("localhost");	//DNS lookup by name
    if (server == NULL) {  //validate
        error2("Error: connection to otp_dec_d\n");
    }    
  
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);  //set up the server addresses, http://www.linuxhowtos.org/C_C++/socket.htm      
    serv_addr.sin_port = htons(portNumber); //connect to the server on the appropriate port
    
    memset(&serv_addr, '\0', sizeof(serv_addr)); //set up destination and source for copying
    if (connect(socketFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ 	//connect to otp_dec_d
        printf("Error: Hull Breach otp_dec_d on port %d\n", portNumber);
		exit(2);	//report hull breach failure
    }

    numSent = write(socketFD, buffer1, cypherLength - 1);	//send cipher to otp_dec_d and validate that it worked
    if (numSent < cypherLength - 1){
        printf("Error: could not send message to otp_dec_d on port %d\n", portNumber);
        exit(2);
    }
    
    memset(buffer3, 0, 1); //clear buffer3 for the acknowledgement
    numReceived = read(socketFD, buffer3, 1);	//get acknowledgement from server and validate
    if (numReceived < 0){
      	error2("Error receiving acknowledgement from otp_dec_d\n");
    }

    numSent = write(socketFD, buffer2, keyLength - 1);	//send key to otp_dec_d and validate
    if (numSent < keyLength - 1){
        printf("Error: could not send key to otp_dec_d on port %d\n", portNumber);
        exit(2);
    }
    
    memset(buffer1, 0, BUFFER_SIZE);  //clear buffer1 for the cipher
    do
    {
        numReceived = read(socketFD, buffer1, cypherLength - 1);	//receive the original message from otp_dec_d, put in buffer 1, and validate
    }
    while (numReceived > 0);
    if (numReceived < 0){
       error2("Error receiving cipher text from otp_dec_d\n");
    }

    for (i = 0; i < cypherLength - 1; i++)
    {
        printf("%c", buffer1[i]);
    }
    printf("\n");
    close(socketFD);

    return 0;
}
