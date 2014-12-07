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
#include <time.h>
#include <sys/time.h>

#define MAX_STRING 80
#define SPREAD_NAME "10030"
#define MAX_MEMBERS 100
#define MAX_MESSLEN 1400
#define NUM_SERVERS 5

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
char              chatroom_join[40];
char              current_room[40];
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
int               you_added = 0;
int               history_line_count = 1;
int               waiting_history = 0;

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
	struct timeval val;
	gettimeofday(&val, NULL);
	sprintf(User, "%ld\n", (val.tv_sec));
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
	printf("\nc [server index]  - Connect to the specified server");
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
			user_name_set = 1;

			/** Send leave update **/
			update_message->type = 6;
			strcpy(update_message->chatroom, current_room);
			strcpy(update_message->user, user);
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);

			/** Set the new username **/
			strcpy(user, input);
			printf("\nYour new username is \'%s\'\n", user);
			you_added = 0;

			/** Send join update **/
			update_message->type = 5;
			strcpy(update_message->user, user);
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);

			break;

		case 'c':
			/** Connects the client to a server's default group **/
			
			/** Check that a user name has been set **/
			if(!user_name_set) {
				printf("\nPlease set your username first\n");
				break;
			}

			chatroom = (char *) malloc(MAX_STRING); 
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
			printf("\nServer group string: %s\n", server_group_string);
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);
			SP_join(Mbox, chatroom_join);

			//Send out the leave update to the server
			update_message->type = 6;
			strcpy(update_message->chatroom, current_room);
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);

			//Join the new chatroom group
			//ret = SP_join(Mbox, chatroom_join);
			SP_leave(Mbox, current_room);
			strcpy(current_room, chatroom_join);
			if(ret < 0) SP_error(ret);
			in_chatroom = 1;


			printf("\nYour new chatroom is \'%s\'\n", chatroom);

			/** Reset chatroom values **/
			Reset_message_array();
			user_list.next = NULL;
			you_added = 0;
			capacity = 0;
			for(int i = 0; i < 25; i++) {
				messages_to_show[i].server_index = -1;
			}

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

			if(!valid(chosen)){
				printf("\nPlease enter a valid line *number*\n");
				break;
			}
			
			if(strcmp(messages_to_show[chosen-1].author, user) == 0) {
				break;
			}

			update_message->type = 1;
			strcpy(update_message->chatroom, current_room);
			update_message->liked_message_lamp.timestamp = messages_to_show[chosen-1].timestamp;
			update_message->liked_message_lamp.server_index = messages_to_show[chosen-1].server_index;
			
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

			strcpy(update_message->chatroom, current_room);
			update_message->type = -1;
			update_message->liked_message_lamp = messages_shown_timestamps[chosen];
			update_message->liked_message_lamp.server_index = messages_to_show[chosen-1].server_index;
			
			//Send out the update to the server
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);

			break;

		case 'h':
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}

			/** Send a complete history request to the server **/
			system("clear");
			printf("HISTORY: \n");
			update_message->type = 3;
			strcpy(update_message->chatroom, current_room);
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1, 
				MAX_MESSLEN, (char *) update_message);
			waiting_history = 1;
			break;

		case 'v':
			/** Print the servers in the current server's network **/
			if(!connected) {
				printf("\nPlease connect to a server first.\n");
				break;
			}

			strcpy(update_message->chatroom, current_room);
			update_message->type = 7;
			SP_multicast(Mbox, AGREED_MESS, server_group_string, 1,
				MAX_MESSLEN, (char *) update_message);
			break;

		case 'p':
			Print_menu();
			break;

		default:
			printf("\nUnknown command\n");
			break;

    }
	if(!waiting_history) {
		printf("\nUser> ");
		fflush(stdout);
	}
	waiting_history = 0;
}

int valid(int line_number)
{
	if((line_number > capacity) || (line_number <= 0))
	{
		printf("\nLINE NUMBER INVALID\n");
		return 0;
	}
	return 1;
}

void Print_messages()
{

	system("clear");
	int chatroom_len = strlen(current_room);
	int mess_len;

	printf("\n------------------------------------");
	printf("\nChatroom: %.*s\n", chatroom_len-1, current_room);
	printf("\nAttendees: ");

	/** Print out who is in the chatroom **/
	user_node *tmp = &user_list;
	while(tmp->next != NULL) {
		printf("%s ", tmp->next->user);
		tmp = tmp->next;
	}

	printf("\n--------------------------------------\n");
	
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
		mess_len = strlen(messages_to_show[i].message);
		printf("%d) %s: %.*s", i+1, messages_to_show[i].author,
			mess_len-1, messages_to_show[i].message);
		if(messages_to_show[i].like_head == NULL) {
			like_count = 0;
			printf("\n");
		}else{
			like_count = messages_to_show[i].num_likes;
		}
		if(like_count != 0) {
			printf("     Likes: %d\n", like_count);
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


	if(Is_regular_mess( service_type )) {
		//Cast the message to message_node
		received_message = *((message_node *) mess);

		/** Deal with a normal message**/
		if(received_message.timestamp >= 0) {

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

			if (lamport >= smallest_lamport) {
				insert(received_message);
				Print_messages();
			}
			else if (lamport < smallest_lamport) {
				int likes = 0;
				
				//Its history - Print the message
				printf("\n%d) %s: %s", history_line_count, 
					received_message.author, received_message.message);

				//Deal with printing likes
				if(received_message.like_head == NULL) {
					likes = 0;
					printf("\n");
				}else{
					likes = received_message.num_likes;
				}

				if(likes != 0) {
					printf("	Likes: %d\n", likes);
				}

				history_line_count++;
				waiting_history = 1;
			}
		}

		/** This is a "user joined chatroom" message **/
		else if(received_message.timestamp == -1) {
			user_node *new_user;
			user_node *head = &user_list;
			new_user = malloc(sizeof(user_node));
			new_user->user = malloc(20);

			strcpy(new_user->user, received_message.author);

			if(strcmp(new_user->user, user) != 0 || !you_added) {
				add_user(head, new_user);
				you_added = 1;
			}
			
			Print_messages();
		}

		/** This is a "user left chatroom" message**/
		else if(received_message.timestamp == -2) {
			user_node *left_user;
			user_node *head = &user_list;
			left_user = malloc(sizeof(user_node));
			left_user->user = malloc(20);

			strcpy(left_user->user, received_message.author);
			remove_user(head, left_user);
			Print_messages();
		}
		
		/** This is a "history done sending" message**/
		else if(received_message.timestamp == -3) {
			//Print the last 25
			for(int i = 0; i < capacity; i++) {
				printf("%d) %s: %s\n", history_line_count,
					messages_to_show[i].author,
					messages_to_show[i].message);
					history_line_count++;
			}
			history_line_count = 1;
			printf("\n-----------------------\n");
		}

		/** This is a message containing the server view **/
		else if(received_message.timestamp == -4) {
			system("clear");
			printf("-------------------------------\n");
			printf("Current Server View:\n");
			for(int i = 1; i < NUM_SERVERS+1; i++) {
				printf("Server %d: ", i);
				if(received_message.message[i] == 1) {
					printf("Connected\n");
				}else{
					printf("Not Connected\n");
				}
			}
			printf("--------------------------------\n");
		}

	}

    else if (Is_membership_mess( service_type )) {
        if (num_groups <= 1) {
            printf("\nConnection failed, server is unresponsive\n");
            SP_leave(Mbox, chatroom);
        } else {
            printf("\nConnection successful!\n");
			connected = 1;
        }
    }
	if(!waiting_history) {
		printf("\nUser> ");
		fflush(stdout);
	}
	waiting_history = 0;
}

int insert(message_node mess) {
    int i, j;
    int lamport;
    int curr_lamport;
  
	lamport = (mess.timestamp * 10) + mess.server_index;
    
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
