/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Client Program
 *
 */

#include "linked_list.c"
#include "sp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "update.h"
#include "user_list.c"

#define MAX_STRING 80
#define SPREAD_NAME "10030"

/** Method Declarations **/
void Print_menu();
int  valid(int);
void Print_messages();

/** Global Variables **/
char              user[MAX_STRING];
char              chatroom[MAX_STRING];
int               server;
int               in_chatroom = 0; //1 if user has chosen a chatroom
user_node         head; //linked list of users in connected chatroom

static char       Spread_name[MAX_STRING] = SPREAD_NAME;
static char       Private_group[MAX_STRING];
static char       User[MAX_STRING];
static mailbox    Mbox;
int               ret;

char              option; //The action the user wants to take
char              *input;

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

	Print_menu();

	/** Deal with User Input **/
	scanf("%c %80c", &option, input);
	switch(option)
	{
		case 'u':
			/** Sets the user's username **/
			//user = input;

		case 'c':
			/** Connects the client to a server's default group **/
			strcpy(chatroom, "default");
			char* server_number = input;
			strcpy(chatroom, server_number);
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			server = atoi(input); //For connection

		case 'j':
			/** Leave the current chatroom and join a new one **/
			SP_leave(Mbox, chatroom);
			//chatroom = input;
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			in_chatroom = 1;

		case 'a':
			/** Create the append update and send it to the chatroom Spread group **/
			if(!in_chatroom) {
				printf("\nMust first connect to a chatroom\n");
				break;
			}
			update_message->type = 0;
			char* message = input;
			//update_message->message = input;
			//Send out the update
			
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
			//Send out the update

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
			//Send out the udpate

		case 'h':
			/** Print the entire chatroom's history stored on the server **/
			if(!in_chatroom) {
				printf("\nMust first conenct to a chatroom\n");
				break;
			}
			//Send request to the server
			//When receive things, just print them on the screen - 
			//should be in order since only one server is sending

		case 'v':
			/** Print the servers in the current server's network **/
			//Send request to the server

		case 'p':
			Print_menu();

		default:
			printf("\nINVALID COMMAND\n");
	}
}

void Print_menu() {
	
	printf("\n =====================");
	printf("\n |     USER MENU     |");
	printf("\n =====================");
	printf("\n   u [name]         - Login with a user name");
	printf("\n   c [sever index]  - Connect to the specified server");
	printf("\n   j [room name]    - Join a chat room");
	printf("\n   a [message]      - Add a message to the chat");
	printf("\n   l [line number]  - Like the message at the line number");
	printf("\n   r [line number]  - Unlike the message at the line number");
	printf("\n   h                - Vuew the entire chat history");
	printf("\n   v                - View the servers in the current server's network");
	printf("\n   p                - Print this menu again");
	printf("\n");

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



