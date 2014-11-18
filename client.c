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

#define MAX_NAME 80

/** Method Declarations **/
void Print_menu();

/** Global Variables **/
char user[MAX_NAME];
char chatroom[MAX_NAME];

int main()
{
	Print_menu();
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
