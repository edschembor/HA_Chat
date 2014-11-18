/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Client Program
 *
 */

#include "sp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "linked_list.c"
#include "update.h"

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

static char       Spread_name[MAX_STRING] = SPREAD_NAME;
static char       Private_group[MAX_STRING];
static char       User[MAX_STRING] = "2";
static mailbox    Mbox;
int               ret;

char              option; //The action the user wants to take
char              input; //The item which will be acted on or with

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
	scanf("%c %80c", &option, &input);
	switch(option)
	{
		case 'u':
			/** Sets the user's username **/
			*user = input;
		case 'c':
			/** Connects the client to a server's default group **/
			strcpy(chatroom, "default");
			char* server_number = input;
			strcpy(chatroom, server_number);
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
			server = (int) input; //For connection
		case 'j':
			/** Leave the current chatroom and join a new one **/
			SP_leave(Mbox, chatroom);
			*chatroom = input;
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
		case 'a':
			/** Create the append update and send it to the chatroom Spread group **/
			update_message->type = 0;
			char* message = input;
			update_message->message = input;
			//Send out the update?
		case 'l':
			/** Create the like update and send it to the chatroom Spread group **/
			chosen = (int) input;
			if(!valid(chosen)) break;
			update_message->type = 1;
			update_message->liked_message_lamport = messages_shown_timestamps[chosen];
			//Send out the update?
		case 'r':
			/** Create the unlike update and send it to the chatroom Spread group **/
			chosen = (int) input;
			if(!valid(chosen)) break;
			update_message->type = -1;
			update_message->liked_message_lamport = messages_shown_timestamps[chosen];
			//Send out the udpate?
		case 'h':
			//TODO - Print server history
		case 'v':
			//TODO - View server config
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
	printf("\nRoom :%c\n", chatroom);
	printf("\nAttendees: ------------\n");
	for(int i = 0; i < 25; i++)
	{
		printf(messages_to_show[(min_message_shown+i)%25];
	}

}



