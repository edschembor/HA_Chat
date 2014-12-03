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
#include <ctype.h>
#include "update.h"

#define MAX_STRING 80
#define SPREAD_NAME "10030"
#define MAX_MEMBERS 100
#define MAX_MESSLEN 1400

/** Method Declarations **/
void Print_menu();
int  valid(int);
void Print_messages();
static void User_command();
static void Read_message();
static void Bye();
int insert(message_node mess);

/** Global Variables **/
char              *user;
char              *chatroom;
char              chatroom_join[200];
char              *server_number;
char              connected_server_private_group[MAX_STRING];

int               server = 0;
int               in_chatroom = 0; //1 if user has chosen a chatroom
int               user_name_set = 0;
int               connected = 0;

user_node         head; //linked list of users in connected chatroom

static char       Spread_name[MAX_STRING] = SPREAD_NAME;
static char       Private_group[MAX_STRING];
static char       User[MAX_STRING];
static mailbox    Mbox;
int               ret;
int               num_groups;
int               capacity; // num elements in messages_to_show

char              option; //The action the user wants to take
char              input[MAX_STRING];

int               min_message_shown;
message_node              messages_to_show[25];
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
			if(!user_name_set) {
				printf("\nPlease set your username first\n");
				break;
			}

			strcpy(chatroom, "default");
            sscanf( &command[2], "%s", input );
			server_number = input;
			strcat(chatroom, server_number);
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			server = atoi(input); //For connection
		    printf("\nConnecting to server %s...\n", input);
			break;

		case 'j':
			/** Leave the current chatroom and join a new one **/
			printf("\nDebug> Joining a chatroom\n");

			/** Check that the client is connected to a server **/
			if(!connected) {
				printf("\nPlease connect to a server first.\n");
				break;
			}
            sscanf( &command[2], "%s", input );
			SP_leave(Mbox, chatroom);
			chatroom = input;
			strcat(chatroom_join, chatroom);
			strcat(chatroom_join, server_number);
			//Chatroom name is of form "<chatroom_name><server_index>"
			//ie) "Room1" is chatroom "Room" on server 1

			//Send out the update to the server
			update_message->type = 5;
			update_message->chatroom = chatroom_join;
			printf("\nDebug> Connected server: %s\n", connected_server_private_group);
			SP_multicast(Mbox, AGREED_MESS, connected_server_private_group, 1,
				MAX_MESSLEN, (char *) &update_message);

			//Join the group
			ret = SP_join(Mbox, chatroom_join);
			SP_leave(Mbox, chatroom);
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
		printf("\n%s",messages_to_show[(min_message_shown+i)%25].message);
	}
}

static void Read_message()
{
	/** Initialize locals **/
	char            target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	message_node    received_message;
	int             endian_mismatch;
	int             service_type;
	char            sender[MAX_GROUP_NAME];
	int16           mess_type;
	char            mess[1500];
    int             lamport, smallest_lamport;

	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,
		&mess_type, &endian_mismatch, sizeof(update), mess);
	
	printf("\nDebug> Client got a message\n");

	if(Is_regular_mess( service_type )) {
		//Cast the message to message_node
		received_message = *((message_node *) mess);
		
		/** Deal with a normal message**/
		if(received_message.timestamp >= 0) {

			if (capacity == 0) {
				insert(received_message);
				return;
			}

			lamport = (received_message.timestamp * 10) + received_message.server_index;
			smallest_lamport = (messages_to_show[0].timestamp * 10) + messages_to_show[0].server_index;

			if (lamport > smallest_lamport) {
				insert(received_message);
				Print_messages();
			}
			else if (lamport < smallest_lamport) {
				//TODO: history
			}
			//TODO: Receive users updates
			//Since they can only be sent from one server, we know what we will be receiving
			//before after we send a request to the server, so our actions here can be 
			//based on that. So, if we requested history, we expect that. If we receive 
			//something else, it is either a user update or a set of 25 messages

		}

		/** Deal with a server private group message **/
		if(received_message.timestamp == -1) {
			printf("\nDebug> Got server private group\n");
			for(int i = 0; i < 80; i++) {
				connected_server_private_group[i] = received_message.message[i];
			}
		}
	}

    else if (Is_membership_mess( service_type )) {
        printf("\nDebug> Cient got a membership message\n");
        if (num_groups <= 1) {
            printf("\nConnection failed, server is unresponsive\n");
            SP_leave(Mbox, chatroom);
        } else {
            printf("\nConnection successful!\n");
			connected = 1;
        }
    }
	printf("\nUser> ");
	fflush(stdout);
}

int insert(message_node mess) {
    int i, j;
    int lamport;
    int curr_lamport;
    if (capacity < 25) {
        messages_to_show[capacity] = mess;
        capacity++;
        return 0; //success
    }
    lamport = (mess.timestamp * 10) + mess.server_index;
    for (i = 0; i < capacity; i++) {
        curr_lamport = (messages_to_show[i].timestamp * 10) + messages_to_show[i].server_index;
        if (lamport <= curr_lamport) {
            if (lamport == curr_lamport) {
                messages_to_show[i] = mess;
                return 0;
            }
            break;   
        }
    }
    //shift values
    for (j = 0; j < i; j++) {
        messages_to_show[j] = messages_to_show[j+1];
    }
    messages_to_show[j+1] = mess;
    return 0;
}

static  void	Bye()
{
	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}
