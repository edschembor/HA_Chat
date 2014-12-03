/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Client Program
 *
 */

#include "data_structure.c"
#include "sp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "update.h"

#define MAX_STRING 80
#define SPREAD_NAME "10030"
#define MAX_MEMBERS 100

/** Method Declarations **/
void Print_menu();
int  valid(int);
void Print_messages();
static void User_command();
static void Read_message();
static void Bye();

/** Global Variables **/
/*char              user[MAX_STRING];
char              chatroom[MAX_STRING];*/
char              *user;
char              *chatroom;

int               server = 0;
int               in_chatroom = 0; //1 if user has chosen a chatroom
int               user_name_set = 0;

user_node         head; //linked list of users in connected chatroom

static char       Spread_name[MAX_STRING] = SPREAD_NAME;
static char       Private_group[MAX_STRING];
static char       User[MAX_STRING];
static mailbox    Mbox;
int               ret;
int               num_groups;

char              option; //The action the user wants to take
char              input[MAX_STRING];

int               min_message_shown;
char              messages_to_show[25][80];
lamport_timestamp messages_shown_timestamps[25];

update            *update_message;

int main()
{
	/** Local Variables **/
	int chosen;

	/** Set up timeout for Spread connection **/
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;

	/** Connect to the Spread client **/
	ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group,
		test_timeout);
	if(ret != ACCEPT_SESSION)
	{
		SP_error(ret);
		exit(0);
	}

	/** All necessary mallocs **/
	update_message = malloc(sizeof(update));
	chatroom = (char *) malloc(MAX_STRING);

	/** Show the user the menu **/
	Print_menu();

	/** Set up the Spread event handling **/
	E_init();

	E_attach_fd(0, READ_FD, User_command, 0, NULL, LOW_PRIORITY);
	E_attach_fd(Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY);

	printf("\nUser> ");
	fflush(stdout);

	E_handle_events();

	return( 0 );
}

void Print_menu() 
{	
	printf("\n=====================");
	printf("\n|     USER MENU     |");
	printf("\n=====================");
	printf("\nu [name]         - Login with a user name");
	printf("\nc [sever index]  - Connect to the specified server");
	printf("\nj [room name]    - Join a chat room");
	printf("\na [message]      - Add a message to the chat");
	printf("\nl [line number]  - Like the message at the line number");
	printf("\nr [line number]  - Unlike the message at the line number");
	printf("\nh                - View the entire chat history");
	printf("\nv                - View the servers in the current server's network");
	printf("\np                - Print this menu again");
	printf("\n---------------------------------------------------------------------\n");
}

static void User_command()
{
    char    command[130];
	char	input[80];
    int     chosen;
	int	i;

	for( i=0; i < sizeof(command); i++ ) command[i] = 0;
	if( fgets( command, 130, stdin ) == NULL ) 
            Bye();

	switch( command[0] )
	{
		case 'u':
			/** Sets the user's username **/
            sscanf( &command[2], "%s", input );
			user = input;
			printf("\nYour new username is \'%s\'\n", user);
			user_name_set = 1;
			break;

		case 'c':
			/** Connects the client to a server's default group **/
			
			/** Check that a user name has been set **/
			/*if(!user_name_set) {
				printf("\nPlease set your username first\n");
				break;
			}*/

			//TODO: Connect to correct server
			strcpy(chatroom, "default");
            sscanf( &command[2], "%s", input );
			char* server_number = input;
			strcat(chatroom, server_number);
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			server = atoi(input); //For connection
			printf("\nConnected to server %s\n", input);
			break;

		case 'j':
			/** Leave the current chatroom and join a new one **/
			printf("\nJoining a chatroom\n");

			/** Check that the client is connected to a server **/
			if(server == 0) {
				printf("\nPlease connect to a server first.\n");
				break;
			}
            sscanf( &command[2], "%s", input );
			SP_leave(Mbox, chatroom);
			chatroom = input;
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			in_chatroom = 1;
			printf("\nYour new chatroom is \'%s\'\n", chatroom);
			break;

		case 'a':
			/** Create the append update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
            sscanf( &command[2], "%s", input );
			update_message->type = 0;
			//update_message->message = input;
			//TODO: Send out the update to the server
			//NEED SERVER PRIVATE GROUP
			break;

		case 'l':
			/** Create the like update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
            sscanf( &command[2], "%s", input );
			chosen = atoi(input);
			if(!valid(chosen)) break;
			update_message->type = 1;
			update_message->liked_message_lamp = messages_shown_timestamps[chosen];
			//TODO: Send out the update to the server
			break;

		case 'r':
			/** Create the unlike update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
            sscanf( &command[2], "%s", input );
			chosen = atoi(input);
			if(!valid(chosen)) {
                printf("\nPlease enter a valid line *number*\n");
                break;   
            }
			update_message->type = -1;
			update_message->liked_message_lamp = messages_shown_timestamps[chosen];
			//TODO: Send out the udpate to the server
			break;
			Read_message();
			break;

		case 'h':
			if(!in_chatroom) {
				printf("\nMust first conenct to a chatroom\n");
				break;
			}
			//TODO: Send request to the server
			//TODO: When receive things, just print them on the screen - 
			//TODO: Should be in order since only one server is sending
			break;

		case 'v':
			/** Print the servers in the current server's network **/
			//TODO: Send request to the server
			break;

		case 'p':
			Print_menu();
			break;

		default:
			printf("\nUnknown command\n");
			Print_menu();

			break;

    }
#if 0
	int chosen;
	
	/** Deal with User Input **/
	scanf("%c %79s", &option, input);
	printf("\nOPTION: %c", option);
	switch(option)
	{
		case 'u':
			/** Sets the user's username **/
			user = input;
			printf("\nYour new username is \'%s\'\n", user);
			user_name_set = 1;
			break;

		case 'c':
			/** Connects the client to a server's default group **/
			
			/** Check that a user name has been set **/
			/*if(!user_name_set) {
				printf("\nPlease set your username first\n");
				break;
			}*/

			//TODO: Connect to correct server
			strcpy(chatroom, "default");
			char* server_number = input;
			strcat(chatroom, server_number);
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			server = atoi(input); //For connection
			printf("\nConnected to server %s\n", input);
			break;

		case 'j':
			/** Leave the current chatroom and join a new one **/
			printf("\nJoining a chatroom\n");

			/** Check that the client is connected to a server **/
			if(server == 0) {
				printf("\nPlease connect to a server first.\n");
				break;
			}
			SP_leave(Mbox, chatroom);
			chatroom = input;
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			in_chatroom = 1;
			printf("\nYour new chatroom is \'%s\'\n", chatroom);
			break;

		case 'a':
			/** Create the append update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
			update_message->type = 0;
			//update_message->message = input;
			//TODO: Send out the update to the server
			//NEED SERVER PRIVATE GROUP
			break;
			
		case 'l':
			/** Create the like update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
			chosen = atoi(input);
			if(!valid(chosen)) break;
			update_message->type = 1;
			update_message->liked_message_lamp = messages_shown_timestamps[chosen];
			//TODO: Send out the update to the server
			break;

		case 'r':
			/** Create the unlike update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
			chosen = atoi(input);
			if(!valid(chosen)) break;
			update_message->type = -1;
			update_message->liked_message_lamp = messages_shown_timestamps[chosen];
			//TODO: Send out the udpate to the server
			break;

		case 'h':
			/** Print the entire chatroom's history stored on the server **/
			if(!in_chatroom) {
				printf("\nMust first conenct to a chatroom\n");
				break;
			}
			//TODO: Send request to the server
			//TODO: When receive things, just print them on the screen - 
			//TODO: Should be in order since only one server is sending
			break;

		case 'v':
			/** Print the servers in the current server's network **/
			//TODO: Send request to the server
			break;

		case 'p':
			Print_menu();
			break;

		default:
			printf("\nINVALID COMMAND\n");
			break;
	}
#endif
	printf("\nUser> ");
	fflush(stdout);
}

int valid(int line_number)
{
	if((line_number > min_message_shown + 25) || (line_number < min_message_shown))
	{
		printf("\nLINE NUMBER INVALID\n");
		return 0;
	}
	return 1;
}

void Print_messages()
{
	printf("\nRoom :%s\n", chatroom);
	printf("\nAttendees: ------------\n");
	for(int i = 0; i < 25; i++)
	{
		printf(messages_to_show[(min_message_shown+i)%25]);
	}
}

static void Read_message()
{
	/** Initialize locals **/
	char      target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	update    received_update;
	int       endian_mismatch;
	int       service_type;
	char      sender[MAX_GROUP_NAME];
	int16     mess_type;
	char      mess[1500];

	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,
		&mess_type, &endian_mismatch, sizeof(update), mess);
	
	printf("\nCLIENT GOT A MESSAGE\n");

	if(Is_regular_mess( service_type )) {
		//Cast the message to an update
		received_update = *((update *) mess);
		
		//TODO: Receive 25 recent
		//TODO: Receive all
		//TODO: Receive users updates
		//Since they can only be sent from one server, we know what we will be receiving
		//before after we send a request to the server, so our actions here can be 
		//based on that. So, if we requested history, we expect that. If we receive 
		//something else, it is either a user update or a set of 25 messages
		
	}

	//TODO: Receive 25 recent
	//TODO: Receive all
	//TODO: Receive users updates
}

static  void	Bye()
{

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}
