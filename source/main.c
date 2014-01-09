/*
 * 
 * David Lettier (C) 2014.
 * 
 * http://www.lettier.com/
 * 
 * Telnet client clone (sort of).
 * 
 * Compiled with gcc version 4.7.2 20121109 (Red Hat 4.7.2-8) (GCC).
 * 
 * Tested on Linux 3.8.11-200.fc18.x86_64 #1 SMP Wed May 1 19:44:27 UTC 2013 x86_64 x86_64 x86_64 GNU/Linux.
 * 
 * To compile: $ gcc main.c -pthread -o telnetClient.out
 * 
 * To run: 
 * 
 * $ ./telnetClient.out [url|ip] [port]
 * 
 * or
 * 
 * $ ./telnetClient.out
 * 
 * Example usage:
 * 
 * $ ./telnetClient.out google.com 80 
 * Trying 173.194.43.35...
 * Connected to google.com.
 * Escape character is '^]'.
 * GET test HTTP/1.0
 * 
 * HTTP/1.0 302 Found
 * Location: http://www.google.com/
 * Cache-Control: private
 * Content-Type: text/html; charset=UTF-8
 * X-Content-Type-Options: nosniff
 * Date: Sun, 29 Sep 2013 02:39:13 GMT
 * Server: sffe
 * Content-Length: 219
 * X-XSS-Protection: 1; mode=block
 * Alternate-Protocol: 80:quic
 * 
 * <HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
 * <TITLE>302 Moved</TITLE></HEAD><BODY>
 * <H1>302 Moved</H1>
 * The document has moved
 * <A HREF="http://www.google.com/">here</A>.
 * </BODY></HTML>
 * Connection closed by foreign host.
 * 
 * telnet>quit
 * $ 
 * 
 * Example usage:
 * 
 * $ ./telnetClient.out
 * telnet> open google.com 80
 * Trying 173.194.43.36...
 * Connected to google.com.
 * Escape character is '^]'.
 * GET test HTTP/1.0
 * 
 * HTTP/1.0 302 Found
 * Location: http://www.google.com/
 * Cache-Control: private
 * Content-Type: text/html; charset=UTF-8
 * X-Content-Type-Options: nosniff
 * Date: Sun, 29 Sep 2013 01:33:53 GMT
 * Server: sffe
 * Content-Length: 219
 * X-XSS-Protection: 1; mode=block
 * Alternate-Protocol: 80:quic
 * 
 * <HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
 * <TITLE>302 Moved</TITLE></HEAD><BODY>
 * <H1>302 Moved</H1>
 * The document has moved
 * <A HREF="http://www.google.com/">here</A>.
 * </BODY></HTML>
 * Connection closed by foreign host.
 * 
 * telnet>close
 * Connection already closed.
 * 
 * telnet>help
 * Commands may be abbreviated.  Commands are:
 * 
 * close			close current connection
 * open [host] [port]	connect to a site
 * quit			exit telnet
 * help			print help information
 * 
 * telnet>quit
 * $
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#define BUFFER_SIZE 1024

void wait( long seconds ) // Similar to sleep().
{
	
	clock_t limit, now = clock();
	
	limit = now + seconds * CLOCKS_PER_SEC;
	
	while ( limit > now )
		now = clock( );
}

int error( char* msg )
{
	
	perror( msg ); // Print the error message to stderr.
    
	exit( 0 ); // Quit the process.
    
}

void sigpipe_handler( int s )
{	
	
	// Ignore.
	
}

void connect_socket( int* sockfd, struct sockaddr_in* serv_addr, struct hostent** server, char* host_name, char* port_number )
{
	
	int port_num = 0;
	
	int i = 0;
	
	char ip[ 100 ];
	
	struct in_addr** addr_list;
	
	port_num = atoi( port_number );
	
	( *sockfd ) = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ); // Create a socket.
	
	if ( ( *sockfd ) < 0 ) 
		error( "ERROR opening socket" );
	
	( *server ) = gethostbyname( host_name ); // Convert URL to IP.
	
	if ( ( *server ) == NULL ) 
		error( "ERROR, no such host" );
	
	// Zero out the server address structure.
	
	bzero( ( char* ) &( *serv_addr ), sizeof( ( *serv_addr ) ) );
	
	( *serv_addr).sin_family = AF_INET;
	
	// Copy the server's IP address to the server address structure.
	
	bcopy( ( char* ) ( *server )->h_addr, ( char* ) &( *serv_addr ).sin_addr.s_addr, ( *server )->h_length );
	
	// Convert the port number integer to network big-endian style and save it to the server address structure.
	
	( *serv_addr ).sin_port = htons( port_num );
	
	// Get server IP and print it to stdout for the user.
	
	addr_list = ( struct in_addr** ) ( *server )->h_addr_list;
					
	for( i = 0; addr_list[ i ] != NULL; i++ )
	{
		
		strcpy( ip, inet_ntoa( *addr_list[ i ] ) );
		
		break;
	
	}
	
	printf( "Trying %s...\n", ip );
	
	// Call up the server using its IP address and port number.
	
	if ( connect( ( *sockfd ), ( struct sockaddr * ) &( *serv_addr ), sizeof( ( *serv_addr ) ) ) < 0 )
	{
		
		// There was an error connecting to the server.
		
		error( "ERROR connecting" );
		
	}
	
}

int write_socket( int* sockfd, char** input_buffer )
{
	
	int return_status = 0; // Write return status.
	
	return_status = write( ( *sockfd ), ( char* ) ( *input_buffer ), strlen( ( *input_buffer ) ), 0 );
	
	signal( SIGPIPE, sigpipe_handler ); // Catch broken pipe, error number 32.
	
	if ( errno == 32 ) // Broken pipe?
	{
		
		// Close the socket.
		
		printf( "Connection closed by foreign host.\n" );
			
		close( ( *sockfd ) ); 
			
		( *sockfd ) = -1;
		
		return 0;
		
	}
	
	if ( return_status < 0 ) // There was an error.
	{
		
		error( "ERROR writing to socket" );
		
	}
	
	return 0;

}

void* read_socket( void* sockfd ) // Thread function.
{
	
	int return_status = 0; // Read return status.
	
	char* output_buffer = malloc( BUFFER_SIZE * sizeof( char ) );
	
	memset( output_buffer, 0, BUFFER_SIZE * sizeof( char ) ); 
	
	while ( 1 )
	{
		
		return_status = read( ( *( ( int* ) sockfd ) ), ( char* ) output_buffer, BUFFER_SIZE * sizeof( char ), 0 );
		
		if ( return_status < 0 ) // There was an error.
		{
			
			error( "ERROR reading from socket" );
			
		}
		else if ( return_status > 0 ) // Got something from the server.
		{
			
			printf( "%s", output_buffer );
		
			memset( output_buffer, 0, BUFFER_SIZE * sizeof( char ) );
			
		}
		else if ( return_status == 0 ) // Connection must be closed.
		{
			
			// Close the socket.
			
			printf( "Connection closed by foreign host.\n" );
			
			close( ( *( ( int* ) sockfd ) ) ); 
			
			( *( ( int* ) sockfd ) ) = -1;
			
			break;
			
		}

	}
	
	return NULL;
	
}

int main( int argc, char* argv[ ] )
{
	
	// Buffers.
	
	char* temp_buffer   = malloc( BUFFER_SIZE * sizeof( char ) );
	
	char* user_input    = malloc( BUFFER_SIZE * sizeof( char ) );
	
	char* user_command; // User command such as help.
	
	char* host_name; // Host name such as google.com.
	
	char* portno; // Port number such as 80.
	
	struct sockaddr_in serv_addr; // Server address data structure.
	struct hostent*    server;    // Server data structure.
	
	int sockfd; // Socket file descriptor.
	
	int quit = 0; // Quit boolean.
	
	int connected = 0; // Connected boolean.
	
	int using_args = 0; // If they put the host and port as arguments to the program.
	
	int i = 0; // Increment.
	
	if ( argc == 3 )
	{
		
		using_args = 1;
		
	}
	
	if ( !using_args )	
		fputs( "telnet>", stdout );
	
	while( quit != 1 ) // Quit when they type quit.
	{
		
		// Get the user input.
		
		if ( !using_args )
		{
			
			fgets( user_input, BUFFER_SIZE, stdin );		

			for( i = 0; i < BUFFER_SIZE; i++ )
			{
				
				// Replace the newline char in the input with the string terminal char.
				
				if ( user_input[ i ] == '\n' )
				{
					
					user_input[ i ] = '\0';
					
				}
				
			}
			
			// Copy the input to a temp buffer as strtok modifies the buffer it is given.
			
			memcpy( temp_buffer, user_input, sizeof( char ) * BUFFER_SIZE );
			
			// Get the command token they typed.
			
			user_command = strtok( temp_buffer, " " );
			
		}
		else
		{
			
			user_command = "open";
			
			host_name = argv[ 1 ];
			
			portno = argv[ 2 ];
			
		}
		
		if ( user_command != NULL ) // Did they type some command?			
		{
		
			if ( strncmp( "help", user_command, 4 ) == 0 ) // Did they type help?
			{
				
				fputs( "Commands may be abbreviated.  Commands are:\n\nclose\t\t\tclose current connection\nopen [host] [port]\tconnect to a site\nquit\t\t\texit telnet\nhelp\t\t\tprint help information\n", stdout );
				
			}
			else if ( strncmp( "open", user_command, 4 ) == 0 ) // Did they type open?
			{

				if ( !using_args )
					host_name = strtok( NULL, " " ); // Get the host name token, that is open<space>host_name<space>port_number.
				
				if ( host_name != NULL ) // Did they give a host name?
				{
					
					if ( !using_args )
						portno = strtok( NULL, " " );
					
					if ( portno != NULL ) // Did they give a port number?
					{
						
						for ( i = 0; i < strlen( portno ); i++ )
						{
							
							// Replace the \n character with the null character.
							
							if ( portno[ i ] == 0x0A )
								portno[ i ] = 0x00;
							
						}
						
						if ( strlen( portno ) > 6 )						
							portno[ 6 ] = 0x00; // Max port number is [0,65535].
							
						using_args = 0;
						
						// Connect to the server.
						
						connect_socket( &sockfd, &serv_addr, &server, host_name, portno );					
						
						printf( "Connected to %s.\n", host_name );
						
						connected = 1;
						
						printf( "Escape character is '^]'.\n" );
						
						// Create a thread to read from the socket.
						
						pthread_t read_thread;
  
						if ( pthread_create( &read_thread, NULL, read_socket, ( void* ) &sockfd ) ) 
						{
							
							error( "ERROR creating thread" );
							
						}
						
						// Enter in send loop where the user can send their input to the server.
						// If user presses ctrl+] then the loop will exit.
						// Otherwise, if either read_socket or write_socket detect a closed connection
						// they'll set the socket file descriptor to -1 and this loop will terminate.
						
						do
						{
							
							// Zero out the user input buffer.
							
							memset( user_input, 0, BUFFER_SIZE * sizeof( char ) );
						
							// Get user input.
							
							fgets( user_input, BUFFER_SIZE, stdin );
							
							if ( user_input[ 0 ] != 0x1d ) // Input does not equal '^]'.
							{
								
								if ( sockfd >= 0 ) // Valid socket file descriptor.
								{
									
									// Write the input out to the socket.
									
									write_socket( &sockfd, &user_input );
									
									wait( 1 ); // Wait a second 'til next input.
									
								}
								else
								{
	
									break;
									
								}
								
							}
							else
							{

								break;
								
							}
							
						}						
						while( sockfd >= 0 ); // While the socket file descriptor is good.
						
					}
					else
					{
					
						printf( "?Invalid command usage\nUsage: open [host name] [port]\n" );
						
					}
					
				}
				else
				{
					
					printf( "?Invalid command usage\nUsage: open [host name] [port]\n" );
					
				}
				
			}
			else if ( strncmp( "close", user_command, 5 ) == 0 )
			{
				
				if ( connected == 0 ) // Not connected?
				{
					
					fputs( "?Need to be connected first.\n", stdout );
					
				}
				else
				{

					if ( sockfd >= 0 ) // Valid socket file descriptor?
					{
					
						// Close socket. Report failure or success.
						
						if ( close( sockfd ) < 0 )
						{
							
							error( "ERROR failed to close connection" );
							
						}
						else
						{
							
							fputs( "Connection closed.\n", stdout );
						
							connected = 0;
						
						}
						
					}
					else
					{
						
						fputs( "Connection already closed.\n", stdout );
						
						connected = 0;
						
					}
				
				}
				
			}
			else if ( strncmp( "quit", user_command, 4 ) == 0 ) // Did they type quit?
			{
				
				quit = 1;
				
				break;
				
			}
			else
			{
				
				fputs( "?Invalid command\n", stdout );
				
			}
			
		}
		else
		{
			
			fputs( "?Invalid command\n", stdout );
			
		}
		
		// Zero out the buffers, resetting them for the next go-around.
		
		memset( temp_buffer,   0, BUFFER_SIZE * sizeof( char ) );
		
		memset( user_input,    0, BUFFER_SIZE * sizeof( char ) );
		
		fputs( "\ntelnet>", stdout );
		
	}

	return 0;
}
