#include <stdio.h>
#include <syslog.h>

int main(int argc, char* argv[])
{
	openlog(NULL, 0, LOG_USER);
	
	// Check the number of arguments
	if (argc < 3)
	{
		syslog(LOG_ERR, "Not enough arguments. Exiting.");
		return 1;
	}
	
	char* writefile = argv[1];
	char* writestr = argv[2];

	// Open the write file and check if it was ok
	FILE *fileptr = fopen(writefile, "w");
	if (fileptr == NULL)
	{
		syslog(LOG_ERR, "File could not be opened. Exiting.");
		return 1;
	}
	
	// Write to write file the write string
	syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
	fprintf(fileptr, "%s", writestr);
	
	// Close the write file
	fclose(fileptr); 
	
	closelog();
	
	return 0;
}
