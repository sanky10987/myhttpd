#define PORT 1062
#define BUFFER_SIZE 1024
#define ROOT "cd ~"
#define NUM_T 10
#define SERVER "webserver/1.0"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define REQUEST_LINE_MAX 1024

#include <stdio.h>
#include <pthread.h>
#include  <string.h>
#include "unp/unp.h"
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/un.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/poll.h>

#include <sys/stat.h>
#include <signal.h>


/**/

pthread_mutex_t count_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ready_queue_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_var   = PTHREAD_COND_INITIALIZER;
pthread_cond_t  condition_var_ready   = PTHREAD_COND_INITIALIZER;


/*Tokenizing part start*/

//variable to check daemonize
int debug=0;

//store file name for logging
int logflag=0;
char logpath[1024]="/log.txt";
//
char dir_Name[1024];
//TODO change default port 8080
int port=8080;
//default queue time
int Q_time=60;
//default number of threads
int threadNum=4;
//default scheduling to FCFS
char* sched="FCFS"; 
int schedmode=1;

/*variables for getting the IP address*/
socklen_t len;
struct sockaddr_storage addr;

struct clientDetails
{
	int fd;
	struct sockaddr_in addr;
	int length;
	char * filename;
	char * version;
	char * type;
	char * ipaddress;
	char * shortfilename;
	char * firstline;
	int status;
	time_t dispatched_time;
	time_t executed_time;
};

struct scheduling_queue
{
	struct clientDetails *task;
	struct scheduling_queue *next;
	struct scheduling_queue *prev;
	int count;
};
struct ready_queue
{
	struct clientDetails *task;
	struct ready_queue *next;
	
};
struct logger
{
	char *ipAdddr;
	char *time_in_q;
	char *time_out;
	char *req_firstline;
	size_t response_length;
	
} logfile;

struct scheduling_queue *head_schedulingqueue,*sloop_ptr;
struct ready_queue *head_readyqueue,*rloop_ptr;

void logDetails(struct clientDetails*);
void *functionExecute();
void *functionDispatcher();
void * listeningSocket();
void * executeRequest();//display output in browser.....//
int tcp_printf( struct clientDetails*, const char *, ... );
void  generateResponse(struct clientDetails*);

void * threadqueue_add_element(int,struct clientDetails* );
//void *  threadqueue_remove_element( struct threadqueue_t*);
struct clientDetails* threadqueue_get_schedulingqueue_head_element();
struct clientDetails* threadqueue_get_readyqueue_head_element();
//struct threadqueue_t* threadqueue_new(void);
//struct threadqueue_t* threadqueue_free( struct threadqueue_t* );

int i=0,sock,scheduling=1;
//sem_t mutex;
int size_q=0;
int k=0;
int count=0;
struct sockaddr_in sockaddr;

int main(int argc,char* argv[])
{
	
	if (getcwd(dir_Name, sizeof(dir_Name)) != NULL){
		fprintf(stdout, "Current working dir: %s\n", dir_Name);
		//changedir(dir_Name);
	}
	
	sprintf(logpath,"%s%s",dir_Name,"/log.txt");
	
	for(i=1;i<argc;i++)
	{
		if (strcmp(argv[i],"-p")==0)
			port = atoi(argv[++i]);
		else if (strcmp(argv[i],"-s")==0)
			strcpy(sched,argv[++i]);
		else if (strcmp(argv[i],"-d")==0)
			debug=1;
		else if (strcmp(argv[i],"-n")==0)
			threadNum = atoi(argv[++i]);
		else if (strcmp(argv[i],"-t")==0)
			Q_time = atoi(argv[++i]);
		else if (strcmp(argv[i],"-l")==0)
		{
			logflag = 1;
			strcpy(logpath,argv[++i]);
		}
		else if(strcmp(argv[i],"-h")==0){
			printf("help mode");
			serverOptions();
		}
		else if(strcmp(argv[i],"-r")==0)
		{
			strcpy(dir_Name,argv[++i]);
			//changedir(dir);
		}
		else{
			printf("\n\nNo Options Passed, printing the server descriptor...\n");
		}
	}
	
	/*if(debug){
		threadNum=1;
	}*/
	
	
	/*printf("\nSERVER DESCRIPTORS \n");
	printf("***************************************************************");
	printf("\nPort No : %d\n",port);
	printf("\nQueue Time : %d\n",Q_time);
	printf("\nNo of the Threads in the Pool : %d\n",threadNum);
	printf("\nScheduling Algorithm : %s\n",sched);
	printf("\n--------------------------------------------------------------\n");*/
	
	
	
	if(debug == 0){
		int pid = fork();
		if(pid < 0){
			printf("fork failed");
		}else{
			if(pid > 0){
				exit(0);
			}
		}
	}else{
		threadNum=1;
	}
	
	
	
	
	
	if(strcmp(sched,"FCFS")==0)
		scheduling=1;
	if(strcmp(sched,"SJF")==0)
		scheduling=2;
	
	
	/**/
	//pthread_create( &dispatcherThread, NULL, &functionDispatcher, NULL);
	
	pthread_t thread1;
	int thread1_return;//int sock
	
	
	//daemon_init(argv[0],0);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;//accept request onany interface.
	sockaddr.sin_port = htons(port);//host to network to convert binary port number
	
	bind(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
	// listen(sock, LISTENQ);//harsha check max num of client conn
	printf("HTTP server listening on port %d\n", port);
	/*while (1)
	 {
	 int s;
	 FILE *f;
	 
	 s = accept(sock, NULL, NULL);
	 printf("entered while --- %d \n",s);
	 if (s < 0) break;
	 
	 f = fdopen(s, "a+");
	 //process(f);
	 fclose(f);
	 }*/
	
	thread1_return = pthread_create(&thread1, NULL, listeningSocket,&sock);
	
	
	/***********creating threads*************/
	
	pthread_t threads[threadNum];
	int rc;
	long t;
	for(t=0; t<threadNum; t++){
		//printf("In main: creating thread %ld\n", t);
		rc = pthread_create(&threads[t], NULL, functionExecute, (void *)t);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	/***********end of creating threads************/
	
	//thread2_return = pthread_create(&thread2, NULL, arrangeSchedule,(void*) client_ptr);
	pthread_join(thread1,NULL);
	//	pthread_join(dispatcherThread,NULL);
	if(thread1_return == 0)
	{}
	close(sock);
	return 0;
}

void* listeningSocket(void *ptr)
{
	struct clientDetails *client_ptr;
	pthread_t dispatcherThread;
	//DIR *dp;
	//struct dirent *dirp;
	//char *ch;
	int n,i=0,size,dispatcherThread_return;
	struct stat st;
	struct sockaddr_in cliaddr;
	char BUF[1024];
	char  type[50],file[50],shortfile[50],ver[50],bufdir[400],tempfile[50];
	//char *req;//="GET/index.html/http1.1";
	int clientfd,clientlength;
	//printf("\ninside thread");
	//printf("\n%d",*((char*)ptr));
	listen(sock, LISTENQ);//harsha check max num of client conn
	
	i++;
	//printf("\n%d",i);
	dispatcherThread_return=pthread_create( &dispatcherThread, NULL, &functionDispatcher, NULL);
	
	while(1)
	{
		//sem_wait(&mutex);
		clientlength=sizeof(sockaddr);
		clientfd=accept(sock,(struct sockaddr*)&cliaddr, &clientlength);
		
		if(clientfd<0)
		{
			perror("accept");
			exit(1);
		}
		if(clientfd > 0)
		{
			n=recv(clientfd,BUF,BUFFER_SIZE,0);//max_size
			if(n==1)
			{
				perror("receive");
			}
			
			
			if(n==0)
			{
				//close(clientfd);
				printf("directory requested");
			}
			printf("\nmessage received= %s",BUF);
			//logfile.req=req;
			//n=strlen(BUF)+1;
			//printf("request is:");
			//puts(req);
			
			sscanf(BUF,"%*s %s",file);
			strcpy(shortfile,file);
			strcpy(tempfile,dir_Name);
			strcat(tempfile,file);
			strcpy(file,tempfile);

			//sprintf(file,"%s%s",dir_Name,file);
			//sem_wait(&mutex);
			if (getcwd(bufdir, sizeof(bufdir)) != NULL)
				fprintf(stdout, "Current working dir: %s\n", bufdir);
			
			stat(file, &st);
			size = st.st_size;
			
			/*dp=opendir("/");
			 while ((dirp=readdir(dp))!=NULL)
			 {
			 stat(dirp->d_name,&st);
			 ch=malloc(strlen(dirp->d_name)+1);
			 ch[0]='/';
			 ch=strcat(ch,dirp->d_name);
			 if(S_ISDIR(st.st_mode))
			 {    
			 printf(" current directory means file is not present\n",file);
			 size=0;	
			 }
			 if(strcmp(file,ch) == 0)
			 {
			 
			 if(!(S_ISDIR(st.st_mode)))
			 {
			 size=(int)st.st_size;
			 printf("%s %d\n", dirp->d_name, size);
			 break;
			 }
			 }//end if of comp
			 }*/
			client_ptr = (struct clientDetails *)malloc(sizeof(struct clientDetails));
			
			
			client_ptr->dispatched_time=time(0);
			client_ptr->addr=cliaddr;
			client_ptr->fd=clientfd;
			client_ptr->length=size;
			client_ptr->filename=file;
			client_ptr->type=type;
			client_ptr->ipaddress=inet_ntoa(cliaddr.sin_addr);
			client_ptr->shortfilename=shortfile;
			printf("filename=%s \n",client_ptr->filename);
			printf("\n  ----------%d ",size);
			client_ptr->version=ver;
			client_ptr->status=1;
			client_ptr->firstline=BUF;
//			printf("\n len=%d",client_ptr->length);
			printf("\nserver connected to client having address %s",inet_ntoa(cliaddr.sin_addr));
			//usleep(2000000);
			
			pthread_mutex_lock( &ready_queue_mutex );
			
			threadqueue_add_element(1,client_ptr);
			count=count+1;
			
			
			if(count>0){
				pthread_cond_signal( &condition_var_ready);
			}
			pthread_mutex_unlock( &ready_queue_mutex );
			/*if(count>3)
			 {
			 dispatcherThread_return=pthread_create( &dispatcherThread, NULL, &functionDispatcher, NULL);
			 }*/
			
		}// end of while
		
	}
	return NULL;
}

/*execute function*/

void *functionExecute()
{
//	printf("executing /n ");
	for(;;)
	{
		// Lock mutex and then wait for signal to relase mutex
		pthread_mutex_lock( &count_mutex );
		// Wait
		// mutex unlocked if condition varialbe in functionCount2() signaled.
		pthread_cond_wait( &condition_var, &count_mutex);
		generateResponse(threadqueue_get_readyqueue_head_element());
		pthread_mutex_unlock( &count_mutex );
		
		// if(count >= COUNT_DONE) return(NULL);
	}
	return NULL;
}


void *functionDispatcher()
{
	struct clientDetails* client_ptr_temp;
	//temp_node=(struct threadqueue_node *)malloc(sizeof(temp_node));
	client_ptr_temp = (struct clientDetails *)malloc(sizeof(struct clientDetails));
	usleep(Q_time);
	for(;;)
	{
		// Lock mutex and then wait for signal to relase mutex
		
		//printf("%d:count",count);
		pthread_mutex_lock( &ready_queue_mutex );
		pthread_cond_wait( &condition_var_ready, &ready_queue_mutex );
		client_ptr_temp=threadqueue_get_schedulingqueue_head_element();
		if(client_ptr_temp==NULL)
		{
			return NULL;
		}
		count--;
		//pthread_mutex_unlock( &count_mutex );
		pthread_mutex_unlock(&ready_queue_mutex);
		
		pthread_mutex_lock( &count_mutex );
		threadqueue_add_element(2,client_ptr_temp);
		pthread_mutex_unlock( &count_mutex );
		pthread_cond_signal( &condition_var);
		//pthread_mutex_unlock(&ready_queue_mutex);
		
		// if(count >= COUNT_DONE) return(NULL);
	}
	return NULL;
}

void * threadqueue_add_element(int a,struct clientDetails* i)
{
	struct clientDetails *temp1 = (struct clientDetails *)malloc(sizeof(struct clientDetails));
	temp1=i;
	if(scheduling == 1)
	{
		if(a == 1)
		{
			struct scheduling_queue *nptr=(struct scheduling_queue *)malloc(sizeof(struct scheduling_queue));
			nptr->prev=NULL;
			nptr->next=NULL;
			nptr->task=temp1;
			nptr->count=++size_q;
			//printf("\n size=%d\n",nptr->task->length);
			if(head_schedulingqueue == NULL)
			{
				//printf("\n HEAD assigned only obnce \n");
				head_schedulingqueue=nptr;
			}
			else
			{
				for(sloop_ptr=head_schedulingqueue;sloop_ptr->next!=NULL;sloop_ptr=sloop_ptr->next)
				{
					
				}
				nptr->prev=sloop_ptr;
				sloop_ptr->next=nptr;
			}
		}
		else
		{
			struct ready_queue *nptr=(struct ready_queue *)malloc(sizeof(struct ready_queue));
			nptr->next=NULL;
			nptr->task=temp1;
			if(head_readyqueue == NULL)
			{
				head_readyqueue=nptr;
			}
			else
			{
				for(rloop_ptr=head_readyqueue;rloop_ptr!=NULL;rloop_ptr=rloop_ptr->next)
				{
				//	printf("\n elements \n");
				}
				rloop_ptr=nptr;
			}
		}
	}
	else
	{
		struct scheduling_queue *nptr=(struct scheduling_queue *)malloc(sizeof(struct scheduling_queue));
		nptr->prev=NULL;
		nptr->next=NULL;
		nptr->task=temp1;
		nptr->count=++size_q;
		//printf("\n size=%d\n",nptr->task->length);
		if(head_schedulingqueue == NULL)
		{
			//printf("\n HEAD assigned only once \n");
			head_schedulingqueue=nptr;
		}
		else
		{
			if(nptr->task->length < head_schedulingqueue->task->length)
			{
			//	printf("smallest head coming in");
				head_schedulingqueue->prev=nptr;
				nptr->next=head_schedulingqueue;
				head_schedulingqueue=nptr;
			}
			for(sloop_ptr=head_schedulingqueue;sloop_ptr!=NULL;sloop_ptr=sloop_ptr->next)
			{
				if(sloop_ptr->next==NULL && (nptr->task->length > sloop_ptr->task->length))
				{
					//printf("at the end");
					sloop_ptr->next=nptr;
					nptr->prev=sloop_ptr;
				}
				else if(sloop_ptr->task->length < nptr->task->length && sloop_ptr->next->task->length > nptr->task->length)
				{
					//printf("in between");
					sloop_ptr->next=nptr;
					nptr->next=sloop_ptr->next;
					sloop_ptr->next->prev=nptr;
					nptr->prev=sloop_ptr;
				}
				
			}
			
		}
	}
}
/* This is a queue and it is FIFO, so we will always remove the first element */
/*void * threadqueue_remove_element( struct threadqueue_t* s )
 {
 printf("\n in remove \n");
 struct threadqueue_node* head = NULL;
 struct threadqueue_node* p = NULL;
 
 if(s->head == NULL && s->tail == NULL)
 {
 printf("Well, List is empty\n");
 }
 else if(s->head == NULL || s->tail == NULL)
 {
 printf("remove:There is something seriously wrong with your list\n");
 printf("One of the head/tail is empty while other is not \n");
 }
 head=s->head;
 p = head->next;
 s->head = p;
 free(head);
 printf("\n removing clothes fully\n ");
 if(p==NULL)
 {
 s->tail = s->head;   /* The element tail was pointing to is free(), so we need an update */
/*	}
 
 return NULL;
 }
 */

/* This is for getting the head element or the first element */
struct clientDetails*  threadqueue_get_schedulingqueue_head_element()
{
	//printf("\n in get head element\n");
	struct scheduling_queue *head = (struct scheduling_queue *)malloc(sizeof(struct scheduling_queue));
	
	if(head_schedulingqueue == NULL)
    {
	//	printf("Well, List is empty\n");
		free(head);
		return NULL;
    }
	else
	{
		head = head_schedulingqueue;
		if(head_schedulingqueue->next != NULL)
		{
			head_schedulingqueue=head_schedulingqueue->next;
			
		}
//		printf("\n getting head \n");
		return head->task;
	}
	return NULL;
}

struct clientDetails*  threadqueue_get_readyqueue_head_element()
{
	//printf("\n in get head element\n");
	struct ready_queue* head = (struct ready_queue *)malloc(sizeof(struct ready_queue));
	
	if(head_readyqueue == NULL)
    {
	//	printf("Well, List is empty\n");
		free(head);
		return NULL;
    }
	
	head = head_readyqueue;
	head_readyqueue=head_readyqueue->next;
	
	//printf("\n getting head \n");
	return head->task;
}

/******************************************************************/
/*********************generating responses*************************/
/******************************************************************/

void generateResponse(struct clientDetails* r)
{
	
	
	FILE *file;
	struct dirent *dirp;
	DIR *dp;
	char line_buffer[1024];
	char html_file[REQUEST_LINE_MAX*10];
	//char html[256];
	const char *mimetype=0;
	const char *fname=0;
	char line[REQUEST_LINE_MAX*10];
	char str[REQUEST_LINE_MAX*10];
	char len[10];
	unsigned int buf_len;
	int is_dir=0;
	
	r->executed_time=time(0);
	//logDetails(r);
	//printf("\n handling the request %s\n", r->filename);
	const char *filetype = strrchr(r->filename,'.');
	puts(filetype);
	//printf("%d",filetype);
	if(filetype) {
		//printf("filetype");
		//filetype++;
		if(!strcmp(filetype,".jpg")) {
			mimetype = "image/jpeg";
		} else if(!strcmp(filetype,".html")) {
			mimetype = "text/html";
		} else if(!strcmp(filetype,".txt")) {
			mimetype = "text/plain";
		}
		
		else {
			mimetype = "application/binary";
		}
	}
	
	
	else{
		
		dp = opendir(r->filename);
		is_dir=1;
        if(dp == NULL){
			r->status=404;
			strcpy(line_buffer,"HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n");
			send(r->fd, line_buffer, strlen(line_buffer), 0);
			
            //perror("Cannot open directory ");
			close(r->fd);
        }
		
		else{
			r->status=200;
            strcpy(line_buffer,"HTTP/1.0 200 OK\nContent-Type:text/html\n\n");
            send(r->fd, line_buffer, strlen(line_buffer), 0);
			char indexfile[1024];
			strcpy(indexfile,r->filename);
			strcat(indexfile,"/index.html");
			
			file = fopen(indexfile,"r");
			
			if(file){
				buf_len = 1; 
			
				while (buf_len > 0) 
				{
					buf_len = fread(line, 1,REQUEST_LINE_MAX*10,file);
					puts(line);
					if (buf_len > 0)  
					{
						//strcat (str,line);
						write(r->fd, line, strlen(line));  
						//printf("nextsend::%d\n",y);
						//printf("%d bytes transferred ..\n", buf_len); 
					}
				}
				fclose(file);
			}
			
			else{
            strcpy(line_buffer,"<h1>Index of Directory </h1>\n\n");
            send(r->fd, line_buffer, strlen(line_buffer), 0);
			
            while ((dirp = readdir(dp)) != NULL){
                if (dirp->d_name[0] != '.'){
                    sprintf(html_file,"<a href =%s%d%s/%s>%s <br>","http://localhost:",port,r->shortfilename,dirp->d_name,dirp->d_name);
                    //strcat(dirp->d_name, "<br>");
                    send(r->fd,html_file,strlen(html_file),0);
					
				}
			}
			}
			closedir (dp);
			close(r->fd);
		}
	}
	//printf("end of filetype");
	
	time_t current = time(0);
	puts(mimetype);
	fname=r->filename;
	//removeChar(fname, '/');
	puts(fname);
	if(!is_dir){
		file = fopen(fname,"r");
	}
	
	
	
	printf("file open");
	if(file && !is_dir) {
		r->status = 200;
		sprintf(len,"%d",r->length);
		strcpy (str,"HTTP/1.1 200 OK\nDate:  ");
		strcat (str,ctime(&current));
		strcat (str,"Server: myhttpd\nLast Modified:  ");
		strcat (str,mimetype);
		strcat (str,"\nContent-Length:");
		strcat (str,len);
		strcat (str,"\n\n");
		int x=send(r->fd,str,strlen(str),0);
//		printf("1st send::%d\n",x);
		
		buf_len = 1; 
		while (buf_len > 0) 
		{
            buf_len = fread(line, 1,REQUEST_LINE_MAX*10,file);
			puts(line);
            if (buf_len > 0)  
            {
				//strcat (str,line);
				write(r->fd, line, strlen(line));  
				//printf("nextsend::%d\n",y);
				//printf("%d bytes transferred ..\n", buf_len); 
            }
		}
		
		
		
		fclose(file);
		close(r->fd);
	} else if(!is_dir){
		r->status = 404;
		tcp_printf(r,"HTTP/1.1 404 File Not Found\n");
		tcp_printf(r,"Date: %s",ctime(&current));
		tcp_printf(r,"Server: wwwserver\n");
		tcp_printf(r,"Connection: close\n");
		tcp_printf(r,"Content-type: text/html\n\n");
		tcp_printf(r,"<b>File not found.</b>\n");
		close(r->fd);
		
		
		
	}
	if(logflag)
	logDetails(r);
	
	
}


int tcp_printf( struct clientDetails *r, const char *fmt, ... )
{
	struct clientDetails* temp= malloc(1* sizeof(temp));
	const char *tempfmt=0;
	temp=r;
	tempfmt=temp;
	va_list args;
	va_start(args,tempfmt);
	//printf("%s",tempfmt);
	char line[1024];
	
	vsnprintf(line,sizeof(line),tempfmt,args);
	puts(line);
	return write(temp->fd,line,strlen(line));
	
	va_end(args);
}


void removeChar(char *str, char garbage) {
	
    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}

void logDetails(struct clientDetails* r){
	printf("log");
	FILE * logfile=fopen(logpath, "a+");
	char logbuf[100];
	char logstatus[100];
	char loglength[100];
	sprintf(logstatus,"%d",r->status);
	sprintf(loglength,"%d",r->length);
	strcpy(logbuf,r->ipaddress);
	strcat(logbuf," ");
	strcat(logbuf,ctime(&r->dispatched_time));
	strcat(logbuf," ");
	strcat(logbuf,ctime(&r->executed_time));
	strcat(logbuf,"\" ");
	strcat(logbuf,r->firstline);
	strcat(logbuf,"\" ");
	strcat(logbuf," ");
	strcat(logbuf,logstatus);
	strcat(logbuf," ");
	strcat(logbuf,loglength);
	
	
	if(logfile){
		fwrite(logbuf, sizeof(logbuf[0]),strlen(logbuf), logfile);
		
		}
	
	else{
		
		printf("error logging");
	}
	fclose(logfile);
	if(debug)
		printf("%s",logbuf);
	
}

int changedir(char *p)
{
	if(chdir(p)!=0) {
		fprintf(stderr,"couldn't change to webdocs directory: %s\n",strerror(errno)), exit(0);		
		return 1;
	}
	return 0;
}


void serverOptions()
{
	printf("\n-------------------------------------------------------------------------------------------------------------\n\n");
	printf("-d : \t\tWeb server will start in debugging mode and will not be daemonize. Server will accept only one connetion at a time. Without this option, the web server will run as a daemon process in the background.\n");
	printf("-h : \t\tPrint a usage summary with all options.\n");
	printf("-l file : \tLog all requests to the given file.\n");
	printf("-p port : \tListen on the given port. If not provided, myhttpd will listen on port 8080.\n");
	printf("-r dir : \tSet the root directory for the http server to dir.\n");
	printf("-t time : \tSet the rear time to time seconds. The default should be 60 seconds.\n");
	printf("-n threadnum: \tSet number of threads waiting ready in the execution thread pool to threadnum. The default should be 4 execution threads.\n");
	printf("-s sched : \tSet the scheduling policy. It can be either FCFS or SJF. The default will be FCFS.\n");
	printf("\n-------------------------------------------------------------------------------------------------------------\n");
	exit(0);
}
