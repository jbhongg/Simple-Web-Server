#include <stdio.h> //Standard input/output library
#include <stdlib.h> //Standard input/output library
#include <string.h> //String processing library
#include <fcntl.h> //File related library
#include <signal.h> //Signal processing library
#include <sys/types.h> //System related library
#include <sys/socket.h> //Network communication library
#include <netinet/in.h> //Library using Internet address system
#include <arpa/inet.h> //Berkeley Socket Usage Library
#include <sys/stat.h> //File information library
#include <pthread.h> //Thread library

#define BUFSIZE 1024 // define buf size
#define HOME ./html/index.html //define home

struct stat s; //Variable to get file size
pthread_mutex_t m_lock; //Declaring mutex objects
int log; //Log file related variables

typedef struct info{
	int *socketfd;
	char *ipaddr;
}c_info;

struct ConentTypeInfo{
   char *ext;
   char *filetype;
};

struct ConentTypeInfo type[4] = {
   {"gif", "image/gif" },  //gif
   {"jpg", "image/jpg"},   //jpg
   {"htm", "text/html" },  //htm
   {"html","text/html" }   //html
}; 

int cgi(char *buf){ //cgi function
   char *ptr; //Variable to extract numbers
   char token[]="=&"; //Set token to extract numbers
   int num1, num2, count, sum=0; //Variable to store the extracted number, Number of numbers, Sum of numbers 

   ptr=strtok(buf,token); // Truncating a string before =
   ptr=strtok(NULL,token); //Cut NNN between = and & and save
   num1=atoi(ptr); //Store ptr in num1 as integer
   strtok(NULL,token); //crop 'to' between & and =
   ptr=strtok(NULL,token); //Cut MMM between & and = and save
   num2=atoi(ptr); //Store ptr in num2 as integer

   count = (num2-num1) + 1; //Counting the number
   sum = ((num1 + num2) * count) >> 1; //sum calculation

   return sum; //sum return
}

void *handling(void *cif) //request handling function
{
	char logbuffer[BUFSIZE]; //log buffer
   struct info *c = (struct info *)cif;
	int *s_fd = c->socketfd; 
	char *ip= c->ipaddr; //Variable to get ip address
	char file_name[BUFSIZE]; //file name
	int ret;
	char buffer[BUFSIZE+1];	

	ret=read(s_fd,buffer,BUFSIZE); //Continue reading from s_fd
	
	if(ret == 0 || ret == -1) { //If reading fails
      close(s_fd); //close s_fd
		return 0;  // exit
	}
	if(ret > 0 && ret < BUFSIZE) //If ret is greater than 0 and less than BUFSIZE
		buffer[ret]=0;   //buffer[ret] becomes 0.
	else buffer[0]=0;   //If the above is not satisfied, buffer[0]=0.

   for(int i=4;i<BUFSIZE;i++) { //Make only the method and path in the buffer, like GET images/05_01.gif
      if(buffer[i] == ' ') { 
         buffer[i] = 0;
         break;
      }
   }

   if( !strncmp(&buffer[0],"GET /\0",6)) //When in the same format as GET /, When there is no requested file
      strcpy(buffer,"GET /index.html");  //Change request to print index.html
   
   int bufflen=strlen(buffer); //Store buffer length in buflen
   char * content = NULL; //String variable to store content type

   for(int i=0; type[i].ext < 4; i++) {
      int len = strlen(type[i].ext); //Specify length
      if(!strncmp(&buffer[bufflen-len], type[i].ext, len)) { //Compare up to the specified number of characters
         content = type[i].filetype; //Formatting, like image/jpg
         break;
      }
   }

   strcpy(file_name,&buffer[5]); //Copy file name
   int fileDescripter = open(file_name,O_RDONLY); //Open file
   fstat(fileDescripter,&s); //Check the information of the file

   if(fileDescripter==-1){ //If not a file
      if(strstr(file_name,"total.cgi")){ //What if there is "total.cgi" in the file name? 
         int n=cgi(&buffer[5]); //Store return value of cgi function in n

         sprintf(buffer,"HTTP/1.1 200 OK\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>%d</H1></BODY></HTML>\r\n",n);
	      write(s_fd, buffer, strlen(buffer)); //Write a buffer to s_fd.
         sprintf(logbuffer, "%s %s %d\n", ip, file_name, (strlen(buffer)-80)); //ip address ,file_name, file size 
         write(log, logbuffer, strlen(logbuffer)); //Write the contents of the buffer to the log.

         close(s_fd); //close s_fd
         free(cif); //free client
         close(fileDescripter); //close fileDescripter
         return 0; //exit
      }

      else{
         sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");
         write(s_fd,buffer,strlen(buffer)); //Write a buffer to s_fd.
         sprintf(logbuffer, "%s %s %d\n", ip, file_name, 9); //ip address ,file_name, file size 
         write(log, logbuffer, strlen(logbuffer)); //Write the contents of the buffer to the log.

         close(s_fd); //close s_fd
         free(cif); //free client
         close(fileDescripter); //close fileDescripter
         return 0; //exit
      }
   }

   else if(S_ISDIR(s.st_mode)){ //When it is a file
      sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");
      write(s_fd,buffer,strlen(buffer)); //Write a buffer to s_fd.
      sprintf(logbuffer, "%s %s %d\n", ip, file_name, 9); //ip address ,file_name, file size 
      write(log, logbuffer, strlen(logbuffer)); //Write the contents of the buffer to the log.

      close(s_fd); //close s_fd
      free(cif); //free client
      close(fileDescripter); //close fileDescripter
      return 0; //exit   
   }

   sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content);
   sprintf(logbuffer, "%s %s %d\n", ip, file_name, s.st_size); //ip address ,file_name, file size 
   write(s_fd,buffer,strlen(buffer)); //Write a buffer to s_fd.
   write(log, logbuffer, strlen(logbuffer)); //Write the contents of the buffer to the log.

   while ((ret = read(fileDescripter, buffer, BUFSIZE)) > 0 ) { //read file
      write(s_fd,buffer,ret);
   }

   close(s_fd); //close s_fd
   free(cif); //free client
   close(fileDescripter); //close fileDescripter
   return 0; //exit  
}

int main(int argc, char *argv[]) //main function
{
   pthread_mutex_init(&m_lock, NULL); //Initializing mutex objects
   pthread_t tid;
	c_info *cif;
	struct sockaddr_in sin, cli; //Structure for using sockets
	int listenfd, clientlen=sizeof(cli);

   log=open("./log.txt", O_CREAT| O_WRONLY | O_TRUNC,0644); // open log file
   
	if(chdir(argv[1]) == -1){ //path, error output on error
		printf("%s error\n",argv[1]);
		exit(4); //exit
	}

	(void)signal(SIGCLD, SIG_IGN); //Signals to parents when any of the child processes are terminated
	(void)signal(SIGHUP, SIG_IGN); //Report disconnection in user terminal

	if((listenfd=socket(AF_INET,SOCK_STREAM,0))<0){ //Create socket file descriptor
		printf("socket error"); //Error message output
		exit(1); //exit
	}

   int option = 1; //Set option value of SO_REUSEADDR to TRUE

   setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); //Code to allow different sockets to bind to the same port
	int port = atoi(argv[2]); //Store the input port value in port
	memset((char*)&sin,'\0',sizeof(sin)); //Initializing
	sin.sin_family=AF_INET; //Communication protocol setting
	sin.sin_port=htons(port);//Process input port number to be recognized by computer 
   sin.sin_addr.s_addr=htons(INADDR_ANY); //Process ip address so that the computer can recognize it

	if(bind(listenfd,(struct sockaddr *)&sin,sizeof(sin))<0){ ////Bind function, error output on error
		printf("bind error"); 
		exit(1); //exit
	}

	if(listen(listenfd,100)<0){ //Waiting for client connection
		printf("listen error"); //Output when listen fails
		exit(1); //exit
	}

	while(1)
	{
      pthread_mutex_lock(&m_lock); //Create lock
		cif=(c_info*)malloc(sizeof(c_info));

		if((cif->socketfd=accept(listenfd,(struct sockaddr *)&cli,&clientlen))<0){ //When connecting to the client, the accept function accepts the connection.
		   printf("accept error");
			return 0; // exit
		}
		
		cif->ipaddr=inet_ntoa(cli.sin_addr); //store ip address
		pthread_create(&tid,NULL,handling,cif); // create thread
		pthread_detach(tid); //Separate thread with identification number tid
      pthread_mutex_unlock(&m_lock); //Unlock
	}

   pthread_mutex_destroy(&m_lock); //Destructing mutex objects
	close(log); // close lof file
	return 0; //exit
}
