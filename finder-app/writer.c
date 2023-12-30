// assignment 2
// Author: Sherif Omar

#include <stdio.h>
#include <syslog.h>

 
int main (int argc, char**argv)
{

    openlog(NULL, 0, LOG_USER);
 
    if(argc < 3)
    {
    	printf("Missing argument.");
    	syslog(LOG_ERR, "Missing argument.");
    	return 1;
    }  
    else 
    {  
        FILE *fp = fopen(argv[1],"w");
	    
        if ( fp != NULL ) 
        {
            fprintf(fp, "%s", argv[2]);
            syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
        }
	else
	{	
	    syslog(LOG_ERR, "File Path [%s] does not exist.\n", argv[1]);
	    return 1;
	}    
    }

    closelog();
    
    
    return 0;
}
