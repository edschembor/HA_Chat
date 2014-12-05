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
void Reset_message_array();
static void User_command();
static void Read_message();
static void Bye();
int insert(message_node mess);

/** Global Variables **/
char              *user;
char              *chatroom;
char              chatroom_join[200];
char              current_room[200];
char              server_number;
char              *server_group_string;

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
message_node      messages_to_show[25];
lamport_timestamp messages_shown_timestamps[25];
user_node         user_list;

update            *update_message;

int main()
{
	/** Local Variables **/
	int chosen;
	server_group_string = malloc(80);

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
	user = malloc(sizeof(160));

    /** Initialize messages_to_show array **/
    int i;
    message_node dummy;
    dummy.timestamp = -1;
    dummy.server_index = -1;
    for (i = 0; i < 25; i++) {
        messages_to_show[i] = dummy;
    }

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
            ret = sscanf( &command[2], "%s", input );
            if (ret < 1) {
                printf(" invalid username \n");
                break;
            }
			strcpy(user, input);
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
            ret = sscanf( &command[2], "%s", input );
            if (ret < 1) {
                printf(" invalid server index \n");
                break;
            }
			server_number = *input;
			strcat(chatroom, &server_number);
			strcpy(current_room, chatroom);
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			server = atoi(input); //For connection

			/** Remember the group of the server you are connecting to **/
			strcpy(server_group_string, "server");
			strcat(server_group_string, &server_number);

		    printf("\nConnecting to server %s...\n", input);
			break;

		case 'j':
			/** Check that the client is connected to a server **/
			if(!connected) {
				printf("\nPlease connect to a server first.\n");
				break;
			}

            ret = sscanf( &command[2], "%s", input );
            if (ret < 1) {
                printf(" invalid chatroom name \n");
                break;
            }
			chatroom = input;
			strcpy(chatroom_join, chatroom);
			strcat(chatroom_join, &server_number);
			
			//Chatroom name is of form "<chatroom_name><server_index>"
			//ie) "Room1" is chatroom "Room" on server 1

			//Send out the join update to the server
			update_message->type = 5;
			strcpy(update_message->chatroom, chatroom_join);
			strcpy(update_message->user, user);
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);

			//Send out the leave update to the server
			//update_message->type = 6;
			//strcpy(update_message->chatroom, current_room);
			//Multicast

			//Join the new chatroom group
			ret = SP_join(Mbox, chatroom_join);
			SP_leave(Mbox, current_room);
			strcpy(current_room, chatroom_join);
			if(ret < 0) SP_error(ret);
			in_chatroom = 1;


			printf("\nYour new chatroom is \'%s\'\n", chatroom);

			/** Reset chatroom values **/
			Reset_message_array();
			//user_list.next = 0;
			capacity = 0;

			break;

		case 'a':
			/** Create the append update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
            /** Overwrites 'a ' part of command[] **/
            int i;
            for (i = 0; i < 80; i++) {
                command[i] = command[i+2];
            }
            command[i] = '\0';
            //sscanf( &command[2], "%s", input );
			
			//Allows spaces in input message
			//if(fgets(input, 80, stdin) == NULL)
			//	Bye();

			update_message->type = 0;
			strcpy(update_message->user, user);
			strcpy(update_message->message, command);
			strcpy(update_message->chatroom, current_room);
			update_message->lamport.server_index = server;
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);
			break;

		case 'l':
			/** Create the like update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
            sscanf( &command[2], "%s", input );
			chosen = atoi(input);
			printf("\nCHOSEN: %d\n", chosen);
			if(!valid(chosen)){
				break;
			}
			update_message->type = 1;
			update_message->liked_message_lamp = messages_shown_timestamps[chosen];
			
			//Send out the update to the server
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);

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
			
			//Send out the update to the server
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);

			break;

		case 'h':
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
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
			break;

    }

	printf("\nUser> ");
	fflush(stdout);
}

int valid(int line_number)
{
	if((line_number > capacity) || (line_number <= 0))
	{
		printf("\nLINE NUMBER INVALID\n");
		return 0;
	}
	printf("\ncapacity: %d\n", capacity);
	return 1;
}

void Print_messages()
{
	printf("\n------------------------------------");
	printf("\nChatroom: %s\n", current_room);
	printf("\nAttendees: ");

	/** Print out who is in the chatroom **/
	user_node *tmp = &user_list;
	while(tmp->next != NULL) {
		printf("%s ", tmp->next->user);
		tmp = tmp->next;
	}

	printf("\n");
	
	int to_show = 0;
	int like_count = 0;
	
	if(capacity <= 25) {
		to_show = capacity;
	}else{
		to_show = 25;
	}

	/** Print out the chatroom messages in the message array **/
	for(int i = 0; i < capacity; i++)
	{
		printf("\n%d) %s: %s", i+1, messages_to_show[i].author,
			messages_to_show[i].message);
		if(messages_to_show[i].like_head == NULL) {
			like_count = 0;
		}else{
			like_count = messages_to_show[i].like_head->counter;
		}
		if(like_count != 0) {
			printf("     Likes: %d", like_count);
		}
	}
	printf("\n--------------------------------------");
}

void Reset_message_array() 
{
	/** Empties the message array of the client **/
	for(int i = 0; i < 25; i++) {
		messages_to_show[i].timestamp = 0;
		strcpy(messages_to_show[i].message, "");
		messages_to_show[i].like_head = NULL;
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
	char            mess[MAX_MESSLEN];
    int             lamport, smallest_lamport;

	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,
		&mess_type, &endian_mismatch, MAX_MESSLEN, mess);

	printf("RETURN : %d", ret);

	printf("\nDebug> Client got a message\n");


	if(Is_regular_mess( service_type )) {
		//Cast the message to message_node
		received_message = *((message_node *) mess);
		
		printf("\nGOT REGULAR MESSAGE\n");

		/** Deal with a normal message**/
		if(received_message.timestamp >= 0) {

			printf("\nGot message with timestamp: %d\n", received_message.timestamp);

			if (capacity == 0) {
				insert(received_message);
				printf("\nInserting message\n");
				Print_messages();
				printf("\nUser> ");
				fflush(stdout);
				return;
			}

			lamport = (received_message.timestamp * 10) + received_message.server_index;
			smallest_lamport = (messages_to_show[0].timestamp * 10) + messages_to_show[0].server_index;

			printf("\nDebug> Lamport: %d\n", lamport);
			printf("\nDebug> Smallest: %d\n", smallest_lamport);

			if (lamport >= smallest_lamport) {
				insert(received_message);
				Print_messages();
			}
			else if (lamport < smallest_lamport) {
				//TODO: history
			}
		}

		/** This is a "user joined chatroom" message **/
		else if(received_message.timestamp == -1) {
			user_node *new_user;
			user_node *head = &user_list;
			new_user = malloc(sizeof(user_node));
			new_user->user = malloc(20);

			printf("\nUser: %s\n", new_user->user);
			printf("\nAuthor %s\n", received_message.author);

			strcpy(new_user->user, received_message.author);

			printf("\nUser to be added: %s\n", new_user->user);
			add_user(head, new_user);
			
			Print_messages();
		}

		/** This is a "user left chatroom" message**/
		else if(received_message.timestamp == -2) {
			//user_node left_user;
			//strcpy(left_user.user, received_message.author);
			//remove_user(user_list, left_user);
			//Print_messages();
		}

	}

    else if (Is_membership_mess( service_type )) {
        printf("\nDebug> Client got a membership message\n");
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
/*    
	if (capacity < 25) {
        messages_to_show[capacity] = mess;
        capacity++;
        return 0; //success
    }
  */  
	lamport = (mess.timestamp * 10) + mess.server_index;
    
	//for (i = 0; i < capacity; i++) {
	for (i = 0; i < 25; i++) {
        if (messages_to_show[i].server_index == -1) {
            messages_to_show[i] = mess;
            capacity++;
            return 0;
        }
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
    for (j = 0; j < i-1; j++) {
        messages_to_show[j] = messages_to_show[j+1];
    }
    messages_to_show[j] = mess;
    return 0;
}

static  void	Bye()
{
	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}
