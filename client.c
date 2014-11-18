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

#define MAX_NAME 80
#define SPREAD_NAME "10030"

/** Method Declarations **/
void Print_menu();

/** Global Variables **/
char           user[MAX_NAME];
char           chatroom[MAX_NAME];
int            server;

static char    Spread_name[MAX_NAME] = SPREAD_NAME;
static char    Private_group[MAX_NAME];
static char    User[MAX_NAME] = "2";
static mailbox Mbox;
int            ret;

char           option;
char           input;

int main()
{
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
			server = (int) input;
			strcpy(chatroom, "default");
			//strcpy(chatroom, input);
			ret = SP_join(Mbox, "chatroom");
			if(ret < 0) SP_error(ret);
		case 'j':
			/** Leave the current chatroom and join a new one **/
			SP_leave(Mbox, chatroom);
			*chatroom = input;
			ret = SP_join(Mbox, chatroom);
			if(ret < 0) SP_error(ret);
		case 'a':
			//TODO - Create update and send
		case 'l':
			//TODO - Create update and send
			//Check the like is valid
		case 'r':
			//TODO - Create update and send
			//Check the unlike is valid
		case 'h':
			//TODO - Print server history
		case 'v':
			//TODO - View server config
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
	printf("\n");

}
