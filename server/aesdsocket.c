

#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <pthread.h>

#define OUTPUT_FILE "/var/tmp/aesdsocketdata"
#define CONNECTION_PENDING_QUEUE 50
#define BUFFER_SIZE 1024

void regsiter_to_SIGINT_SIGTERM();

bool run_as_daemon(int argc, char**argv);

void start_as_daemon();

int fd_global = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t timestamp_thread_id;

struct thread_linked_list_node
{
	pthread_t thread_id;
	bool completion_flag;
	struct thread_linked_list_node *next;
} ;

struct thread_args
{
	int connection_fd;
	struct thread_linked_list_node *thread_info;
	struct sockaddr_in addr_in;
} ;


void* timestamp_thread (void *args)
{
	while(1)
	{	
		sleep(10);
		time_t now = time(NULL);
		struct tm* tm_info = localtime(&now);
		char timestamp[64];
		strftime(timestamp, sizeof(timestamp), "Writing timestamp: %a, %d %b %Y %T %z\n", tm_info);
		pthread_mutex_lock (&mutex);	
		int fd_write = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);	
		write(fd_write, timestamp, strlen(timestamp));
		close(fd_write);
		pthread_mutex_unlock (&mutex);
		
	}
	pthread_exit(NULL);
}


void* handle_accepted_connection (void *arg)
{	
	struct thread_args *argv = (struct thread_args *)(arg);
	int cfd = argv->connection_fd;
	argv->thread_info->completion_flag = false;
	char buf[BUFFER_SIZE];
	ssize_t nbytes;
	while ((nbytes = recv(cfd, buf, sizeof(buf), 0)) > 0)
	{
		pthread_mutex_lock (&mutex);
		int fd_write = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		write(fd_write, buf, nbytes);
		close(fd_write);
		
		if (memchr(buf, '\n', nbytes) != NULL)
		{	
			// packet is complete, string has a new line character
			int fd_read = open(OUTPUT_FILE, O_RDONLY);
			char buffer_to_send[BUFFER_SIZE];
			ssize_t nbytes_to_send;
			while ((nbytes_to_send = read(fd_read, buffer_to_send, sizeof(buffer_to_send))) > 0)
			{
				syslog(LOG_DEBUG, "Read %d bytes from tmp file", (int) nbytes_to_send);
				int sent_bytes = send(cfd, buffer_to_send, nbytes_to_send, 0);   		
				if (sent_bytes <= 0) 
				{
					// got error or connection closed by client
					if (sent_bytes == 0) 
					{
					    	// connection closed
					    	printf("selectserver: socket %d hung up\n", cfd);
					} 
					else 
					{
				    		perror("send");
					}
					close(cfd); 
					syslog(LOG_DEBUG, "Closed connection from %s\n", inet_ntoa(argv->addr_in.sin_addr));
				 }
				 else
				 {
					// no error from send
					syslog(LOG_DEBUG, "Sent %d bytes from %s\n", sent_bytes, inet_ntoa(argv->addr_in.sin_addr));	 
				 }					
			}
			close(fd_read);	
		}
	}
	if (nbytes == 0)
	{
		// connection closed
	    	printf("selectserver: socket %d hung up\n", cfd);
	    	argv->thread_info->completion_flag = true;
	    	free(argv);
	    	pthread_mutex_unlock (&mutex);
	}
	else 
	{
		perror("recv");
	}
	pthread_exit(NULL);
}


int main (int argc, char**argv)
{	
	openlog(NULL, 0, LOG_USER);
	
	// Register for SIGINT and SIGTERM
	regsiter_to_SIGINT_SIGTERM();
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    //IPV4 or IPV6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;	
	struct addrinfo *servinfo, *struct_arr_index;
	
        int status = getaddrinfo(NULL, "9000", &hints, &servinfo);
	if (status != 0)
	{
		exit(EXIT_FAILURE);	
	}
	
	/* getaddrinfo() returns a list of address structures.
	Try each address until we successfully bind(2).
	If socket(2) (or bind(2)) fails, we (close the socket
	and) try the next address. */
	int sfd = -1;
	for (struct_arr_index = servinfo; struct_arr_index != NULL; struct_arr_index = struct_arr_index->ai_next) {
		sfd = socket(struct_arr_index->ai_family, struct_arr_index->ai_socktype, struct_arr_index->ai_protocol);
		fd_global = sfd;
		if (sfd == -1)
		   continue;

		if (bind(sfd, struct_arr_index->ai_addr, struct_arr_index->ai_addrlen) == 0)
		   break;                  /* Success */

		close(sfd);
	}	
	
	freeaddrinfo(servinfo);           /* No longer needed */

	if (struct_arr_index == NULL) 
	{       /* No address succeeded */
		exit(EXIT_FAILURE);
	}

	// check if needs to be run as a deamon
	if (run_as_daemon(argc, argv))
		start_as_daemon();

	status = listen(sfd, CONNECTION_PENDING_QUEUE); 
	if (status == -1)
	{
		exit(EXIT_FAILURE);	
	}

	if (pthread_create(&timestamp_thread_id, NULL, &timestamp_thread, NULL) != 0)
	{	
		return -1;
	}

	struct thread_linked_list_node *head = NULL, *current = NULL;	
	
	struct sockaddr_in addr_in;
	socklen_t addrlen;
        while(1)
        {	      
        	addrlen = sizeof(addr_in);
		int cfd = accept(sfd, (struct sockaddr *) &addr_in, &addrlen);		
		
		if (cfd == -1)
		{	
			exit(EXIT_FAILURE);	
		} 
		else
		{	
			syslog(LOG_DEBUG, "Accepted connection from %s\n", (const char *) inet_ntoa(addr_in.sin_addr));
		}
		
		// creating new node in linked list
		struct thread_linked_list_node *new_node = (struct thread_linked_list_node *) malloc(sizeof(struct thread_linked_list_node));
		new_node->completion_flag = false;	
		new_node->next = NULL;
		
			
		// thread creation 
		pthread_t th_id;
		struct thread_args *args = (struct thread_args *) malloc(sizeof(struct thread_args));
		args->connection_fd = cfd;
		args->thread_info = new_node;
		args->addr_in = addr_in;
		
		if (pthread_create(&th_id, NULL, &handle_accepted_connection, args) != 0)
		{
			return -1;
		}		
		new_node->thread_id = th_id;
			
		
		if(head == NULL)
		{
			head = current = new_node;
		}
		else 
		{
			// add new node to the linked list
			current->next = new_node;
			current = new_node;		
		}
		
		struct thread_linked_list_node *prev = NULL, *current_loc = head;
		// Check if thread if complete and remove from list and join after completion
		while(current_loc != NULL)
		{
			if(current_loc->completion_flag)
			{
				if(pthread_join(current_loc->thread_id, NULL) != 0)
				{
					printf("pthread_join returned error.\n");
				}
				else
				{
					struct thread_linked_list_node *temp = current_loc;
					// free linked list from this thread entry
					if (current_loc == head)
					{
						current_loc = head = head->next;
					}
					else 
					{
						prev->next = current_loc->next;
						current_loc = current_loc->next;					
					}
					free(temp);
				}	
			}	
			else
			{
				prev = current_loc;
				current_loc = current_loc->next;			
			}	
		}
        }
        
        pthread_join(timestamp_thread_id, NULL);

	return 0;
}

bool run_as_daemon(int argc, char**argv)
{
	if (argc == 1) return false;
	else if (argc == 2 && strcmp(argv[1], "-d") == 0)
	{
		return true;
	}
	else
	{
		printf("Wrong options are provided.\n");
		printf("Usage: ./aesdsocket [-d]\n");
		exit(EXIT_FAILURE);
	}
}

void start_as_daemon()
{
	int pid = fork();
	
	if (pid < 0)
	{
		exit(EXIT_FAILURE);
	}
	
	if (pid > 0)
	{
		// parent process
		syslog(LOG_DEBUG, "This is the parent");
		exit(EXIT_SUCCESS);
	}	
	
	// child process
	syslog(LOG_DEBUG, "This is the child");
	if(setsid() < 0)
	{
		exit(EXIT_FAILURE);
	}
		
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

void signal_handler(int signal_number)
{
	if (signal_number == SIGINT || signal_number == SIGTERM)
	{	
		syslog(LOG_DEBUG, "Caught signal, exiting");
		unlink(OUTPUT_FILE);
		shutdown(fd_global, SHUT_RDWR);
		closelog();		
	}
	
	// cleanup
    	pthread_mutex_destroy(&mutex);
}

void regsiter_to_SIGINT_SIGTERM()
{	
	struct sigaction new_action;
	memset(&new_action, 0, sizeof(struct sigaction));
	new_action.sa_handler = signal_handler;
	int reg_status = sigaction(SIGINT, &new_action, NULL);
	if (reg_status == -1)
	{
		// Registeration to SIGINT NOK
		exit(EXIT_FAILURE);
	}
	reg_status = sigaction(SIGTERM, &new_action, NULL);
	if (reg_status == -1)
	{
		// Registeration to SIGTERM NOK
		exit(EXIT_FAILURE);
	}	
}



