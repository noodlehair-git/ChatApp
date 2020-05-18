/*****************************
ASSIGNMENT 4: PROBLEM 2
******************************/

#define _GNU_SOURCE // F_SETSIG
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <errno.h>     


void ready();
void request_received();
void request_initiate();
void communicate();
void quit_handler(int signo);

struct sockaddr_in servaddr, cliaddr, newcliaddr;
int sockfd,tries;
char readbuf[55],sendbuf[55], buf[50],*arg,ans[2],payload[5], payload2[5];
char request[100], request_dup[100], data[30];
int read_bytes,req;
int j,i,k;
char* cmd[2], *file, *f, ip[15], port[10];
int flag, flag2;
int count, done, io, two_req, lower, upper, range, check;
unsigned int key[4];
struct ifreq ifr;

//Random key generator
void printRandoms(int lower, int upper, int range) 
{ 
    int i; 
    for (i = 0; i < range; i++) { 
        int num = (rand() % (upper - lower + 1)) + lower; 
        //printf("%d ", key[i]); 
	key[i]=num;
	//printf("%d ", key[i]);
    } 
} 

//IO handler for an established connection
void terve_msg_receive(){
	signal(SIGQUIT, quit_handler);
	printf("\n#im in comm handler");

	socklen_t length = sizeof(servaddr);
	memset(&readbuf, '\0', sizeof(readbuf));
	if( read_bytes = recvfrom(sockfd, (char *)readbuf, sizeof(readbuf), 0, (struct sockaddr *) &newcliaddr, &length)> 0){
                        //getsockname(sockfd, &newcliaddr, &length);
			if((readbuf[0]=='5')){
				printf("\n#session initiation request from: %s %d",inet_ntoa(newcliaddr.sin_addr), ntohs(newcliaddr.sin_port));
		        }
		        if((readbuf[0]=='9')&&(readbuf[1]== key[0])&&(readbuf[2]==key[1])&&(readbuf[3]==key[2])&&(readbuf[4]==key[3]))
		     		exit(0);
		        if((readbuf[0]=='8')&&(readbuf[1]== key[0])&&(readbuf[2]==key[1])&&(readbuf[3]==key[2])&&(readbuf[4]==key[3])){
                                if((readbuf[5]!='\n')&&(readbuf[5]!=' '))
				        printf("\n#received msg: %.*s", 50, readbuf+5);
			memset(&readbuf, '\0', sizeof(readbuf));   
                        }    
	}

}

//IO handler for race condition (simultaneous request)
void req_handler(){
        two_req=1;
        socklen_t length = sizeof(servaddr);
        if(recvfrom(sockfd, (char *)payload2, sizeof(payload), 0, (struct sockaddr *) &newcliaddr, &length)>0)
                printf("\n#session initiation request from: %s %d",inet_ntoa(newcliaddr.sin_addr), ntohs(newcliaddr.sin_port));
        //sending a reject request to third party 
        payload[0]='7';
        sendto(sockfd, (char *)payload, sizeof(payload), 0, (struct sockaddr *) &newcliaddr, sizeof(newcliaddr));					
}

//alarm handler 
void alarm_handler(int signo) {
        count=1;
	//test: printf("\n#im in alarm handler");
	tries++;
	if(tries>1){
		tries=0;
		ready();
	}
	else
		request_initiate();
}

//Termination condition handler(if user presses ctrl + \)
void quit_handler(int signo) {
        printf("\n#im in SIGQUIT\n");
        payload[0] = '9';    
        payload[1] = key[0];
        payload[2] = key[1];
        payload[3] = key[2];
        payload[4] = key[3];
        sendto(sockfd, (char *)payload, sizeof(payload), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
        exit(0);
}

//IO handler for handling incoming requests from another party
void io_handler(int signo) {
signal(SIGQUIT, quit_handler);
socklen_t length = sizeof(servaddr);
        if((flag==0)&&(io==0)){
		printf("\n#im in io handler");
		//request_received();
                req=1;
                io=1;
        }
        else    
                communicate();
        if(flag2==1)
		communicate();
}

//initial state funtion during which a party can send or receive a request
void ready(){
        //quit handler
	printRandoms(lower, upper, range);
        signal(SIGQUIT, quit_handler);
        alarm(0);

        //setting up IO handler for incoming requests
        struct sigaction handler;
        //Set signal handler for SIGIO 
        handler.sa_handler = io_handler;
        handler.sa_flags = 0;

        //Create mask that mask all signals 
        if (sigfillset(&handler.sa_mask) < 0) 
                printf("sigfillset() failed");
        //No flags 
        handler.sa_flags = 0;

        if (sigaction(SIGIO, &handler, 0) < 0)
                printf("sigaction() failed for SIGIO");

        //We must own the socket to receive the SIGIO message
        if (fcntl(sockfd, F_SETOWN, getpid()) < 0)
                printf("Unable to set process owner to us");

        //Arrange for nonblocking I/O and SIGIO delivery
        if (fcntl(sockfd, F_SETFL, O_NONBLOCK | FASYNC) < 0)
                printf("Unable to put client sock into non-blocking/async mode");
        
	tries=0;
        flag=0;

        //blocking on user input
        printf("\n#ready: ");
        fflush(stdout);
        fgets(request, 30, stdin);
        if(req==1)
                request_received();
        fflush(stdin);
        int len = strlen(request);        
        request[len-1] = '\0';
        strcpy(request_dup, request);

        //tokenizing request into IP address and port number
        //for(i=0;request[i]!=' ';i++)
		//ip[i]=request[i];
        //printf("\n ip: %s", ip);
        //for(int j=i;request[j]!='\0';j++)
		//port[j]=request[j];
        //printf("\n port: %s", port);

        
        //strtok function to tokenize the string
        file = strtok(request, " ");

        //f points to the first argument in the request
        f=file;
        i=0;
        
        while (file != NULL){
         
                        
                cmd[i]=file;
                     
                file = strtok(NULL, " "); 
                //printf("\n loop: %s", cmd[1]);
                i++;
       }
        
   
        //printf("\n ip: %s", cmd[0]);
        //printf("\n port: %s", cmd[1]);

        //assigning port and IP address of the other party
        cliaddr.sin_family = AF_INET;  
        cliaddr.sin_addr.s_addr = inet_addr(cmd[0]); 
        cliaddr.sin_port = htons(atoi(cmd[1]));

        //transfering the control to request_initiate()
        request_initiate();
}


//the user enters this function if he initiates the request
void request_initiate(){
        //quit handler
        signal(SIGQUIT, quit_handler);
        flag=1;
	
	socklen_t length = sizeof(servaddr);
        printf("\n#im in request_initiate");
	payload[0] = '5';
        payload[1] = key[0];
        payload[2] = key[1];
        payload[3] = key[2];
        payload[4] = key[3];

        fcntl(sockfd, F_SETFL, O_NONBLOCK);
     
        if(count<1){
		alarm(10);
                count++;
	      	sendto(sockfd, (char *)payload, sizeof(payload), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));

                //waiting condition for data packet to be received
		while(recvfrom(sockfd, (char *)payload2, sizeof(payload), 0, (struct sockaddr *) &cliaddr, &length)<0){
                //do nothing
		}

                //Control bit 6 means that request was accepted
	    	if((payload2[0]=='6')&&(payload[1] == key[0])&&(payload[2] == key[1])&&(payload[3] == key[2])&&(payload[4] == key[3])){
	    		
	    		printf("\n#success: %s",request_dup);
		        alarm(0);
		        communicate();       
		                            		      	
	    	}
                //Control bit 5 means that request to connect has arrived
		//else if(payload[0]=='5'){
			//printf("\n#session initiation request from: %s %d",inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
                        //alarm(0);
                //}
		else{
                        printf("\n#failure: %s %d",inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
			//count=0;
                    	ready();
                        
                }
              
	}
        //if the other party is not responding, control is shifted back to ready
       else{    
		signal(SIGALRM, alarm_handler);
		count=0;
                printf("\n#failure: %s %d",inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
		ready();
                
        }
	
                      


}

//the user enters this function if he receives the request
void request_received(){
        //quit handler
	signal(SIGQUIT, quit_handler);
        req=0;
        struct sigaction handler;

        //Set signal handler for SIGIO (if a request from third party arrives)
        handler.sa_handler = req_handler;
        handler.sa_flags = 0;

        //Create mask that mask all signals 
        if (sigfillset(&handler.sa_mask) < 0) 
                printf("sigfillset() failed");
        //No flags
        handler.sa_flags = 0;

        if (sigaction(SIGIO, &handler, 0) < 0)
                printf("sigaction() failed for SIGIO");

        //We must own the socket to receive the SIGIO message 
        if (fcntl(sockfd, F_SETOWN, getpid()) < 0)
                printf("Unable to set process owner to us");

        //Arrange for nonblocking I/O and SIGIO delivery */
        if (fcntl(sockfd, F_SETFL, O_NONBLOCK | FASYNC) < 0)
                printf("Unable to put client sock into non-blocking/async mode");

	socklen_t length = sizeof(servaddr);
        printf("\n#im in request_received");
	if(recvfrom(sockfd, (char *)payload2, sizeof(payload), 0, (struct sockaddr *) &cliaddr, &length)>0){
		printf("\n#session initiation request from: %s %d ",inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
		fgets(ans, 2, stdin);
                flag2=0;
                if(two_req==1){
 			printf("\nDo you want to accept the first request?(Second will be ignored)\n\nPress 'y' or 'n': ");
			fgets(ans, 2, stdin);
		}
                //if user accepts the request
                if (ans[0]=='y') {
	    		if((payload2[0]=='5')&&(payload2[1] == key[0])&&(payload2[2] == key[1])&&(payload2[3] == key[2])&&(payload2[4] == key[3])){
				payload[0]='6';
				payload[1] = key[0];
				payload[2] = key[1];
				payload[3] = key[2];
				payload[4] = key[3];
                                //sending 'ok, lets talk' control bit
	    			sendto(sockfd, (char *)payload, sizeof(payload), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                                printf("\n#success: %s %d", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
				fgetc(stdin);
				communicate();
				//break;
				      	
	    		}
        	}
                //if user presses 'n' or any other key
                else {
	    		if((payload2[0]=='5')&&(payload[1] = '1')&&(payload[2] = '2')&&(payload[3] = '3')&&(payload[4] = '4')){
                                //sending 'failure' control bit
				payload[0]='7';
                                payload[1] = key[0];
				payload[2] = key[1];
				payload[3] = key[2];
				payload[4] = key[3];
	    			sendto(sockfd, (char *)payload, sizeof(payload), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));               
				fgetc(stdin);
                                if(recvfrom(sockfd, (char *)payload2, sizeof(payload), 0, (struct sockaddr *) &newcliaddr, &length)>0)
					printf("\n#session initiation request from: %s %d",inet_ntoa(newcliaddr.sin_addr), ntohs(newcliaddr.sin_port) );
				ready();
                                //break;
                         }
                }
			
    	}
	
}

//funtion for actual communication between two parties
void communicate(){
	io=1;
	done=1;
	setbuf(stdout,NULL);
	check=1;
        //quit handler
        signal(SIGQUIT, quit_handler);

        //test: printf("\n#im in comm1");

        //IO handler for communication
	struct sigaction handler;
        //Set signal handler for SIGIO 
        handler.sa_handler = terve_msg_receive;
        //Create mask that mask all signals
        if (sigfillset(&handler.sa_mask) < 0) 
                printf("sigfillset() failed");
        
        handler.sa_flags = 0;

        if (sigaction(SIGIO, &handler, 0) < 0)
                printf("sigaction() failed for SIGIO");

        //We must own the socket to receive the SIGIO message
        if (fcntl(sockfd, F_SETOWN, getpid()) < 0)
                printf("Unable to set process owner to us");

        //Arrange for nonblocking I/O and SIGIO delivery 
        if (fcntl(sockfd, F_SETFL, O_NONBLOCK | FASYNC) < 0)
                printf("Unable to put client sock into non-blocking/async mode");
        
		alarm(0);
		flag=1;
		socklen_t length = sizeof(servaddr);
                
		while(1){
                        printf("\n#your msg: ");
                        memset(&buf, 0, sizeof(buf));
			memset(&sendbuf, 0, sizeof(sendbuf));
                        fgets(buf, 100, stdin);
                        sendbuf[0]='8';
			sendbuf[1] = key[0];
			sendbuf[2] = key[1];
			sendbuf[3] = key[2];
			sendbuf[4] = key[3];
                        strcat(sendbuf, buf);
                        signal(SIGQUIT, quit_handler);
                        sendto(sockfd, (char *)sendbuf, 55, 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                        fgetc(stdin);
			check=0;
                }
		
}

int main(int argc, char** argv) {
   char ip[20];
   done=0;
   req=0;
   tries=0;
   flag=0;
   flag2=0;
   count=0;
   io=0;
   two_req=0;
   lower =0;
   upper =9;
   range =4;
   check =0;
   //printRandoms(lower, upper, range);
   //Creating socket descriptor (AF_INET for adress family of IPv4, SOCK_DGRAM for UDP, 0 for IP)
   if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    //Clearing out the servaddr
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(sockfd, SIOCGIFADDR, &ifr);
    strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    // Filling server information 
    // 0 for port nyumber as we want the OS to assign the 'not in use' ephemeral port
    //ip address of server entered as the command line argument
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = inet_addr(ip); 
    servaddr.sin_port = htons(atoi(argv[1]));
 
    printf("\n IP ADDRESS: %s\n", ip);
	    
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
  
   //finding the length of the servaddr 
   socklen_t length = sizeof(servaddr);

   //setting up the alarm & quit hand
   signal(SIGALRM, alarm_handler);
   signal(SIGQUIT, quit_handler);
   ready();
     
   //closing the socket descriptor
   close(sockfd);
   return 0;
}
