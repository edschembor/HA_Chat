/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Client Program
 *
 */

#include "sp.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_NAME 80
#define SERVER_GROUP_NAME "servers"
#define SPREAD_NAME "10030"
#define NUM_SERVERS 5

/** Method Declarations **/

/** Global Variables **/
static int       machine_index;

static char      Spread_name[MAX_NAME] = SPREAD_NAME;
static char      Private_group[MAX_NAME];
static char      server_group[MAX_NAME] = SERVER_GROUP_NAME;
static char      User[MAX_NAME] = "1";
static mailbox   Mbox;
int              ret;

int              entropyMatrix[NUM_SERVERS][NUM_SERVERS];

int              lamport_counter;

int main(int argc, char *argv[])
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

	/** Join the server group **/
	ret = SP_join(Mbox, server_group);
	if(ret < 0) SP_error(ret);

}

static void Handle_messages()
{
	
	/** Receive a message **/

}

int isMax(int vector[])
{
	/** Checks if the server for the given anti-entropy vector is the server
	 *  with the most updates **/
	int server_val = vector[machine_index - 1];
	for(int i = 0; i < NUM_SERVERS; i++)
		if(vector[i] > server_val)
			return 0;
	
	return 1;

}

int minVal(int vector[])
{
	/** Gets the minimum value in the array - for removing updates  **/
	int min = vector[0];
	for(int i = 1; i < NUM_SERVERS; i++)
		if(vector[i] < min)
			min = vector[i];

	return min;
}
