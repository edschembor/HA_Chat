/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Server Program
 *
 */

#include "sp.h"
#include <stdio.h>
#include <stdlib.h>
#include "update.h"

#define MAX_NAME 80
#define SERVER_GROUP_NAME "servers"
#define SPREAD_NAME "10030"
#define NUM_SERVERS 5
#define MAX_MEMBERS 100

/** Method Declarations **/
static void Handle_messages();
void Send_Merge_Updates();
int Is_Max(int[]);
int Min_Val(int[]);
void Send_All_Messages();
void Send_Recent_Twenty_Five();
void Clear_Updates();

/** Global Variables **/
static int       machine_index;

static char      Spread_name[MAX_NAME] = SPREAD_NAME;
static char      Private_group[MAX_NAME];
static char      server_group[MAX_NAME] = SERVER_GROUP_NAME;
static char      User[MAX_NAME] = "1";
static mailbox   Mbox;
int              ret;

int              entropy_matrix[NUM_SERVERS][NUM_SERVERS];
int              entropy_received = 0;

int              lamport_counter;

chatroom_node    head; //The main data structure
//Need a linked list of users for each

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

	/** Initialize locals **/
	char    target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	update  received_update;
	int     endian_mismatch;
	int     service_type;
	char    sender[MAX_GROUP_NAME];
	int     num_groups;
	int16   mess_type;
	char    mess[1500];
	
	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups,
		target_groups, &mess_type, &endian_mismatch, sizeof(update), mess);
	
	if(Is_regular_mess( service_type )) {
		//received_update = (received_update) mess;
		if(received_update.type == -1) {
			//Perform the unlike update on linked list
			//Put in updates array
			//Compare timestamp w/ yours
			//Increase entropy vector
			//TODO
			//Multicast the update to all servers - INFINITE LOOP
			//Only multicast updates if from a client group
		}
		else if(received_update.type == 1) {
			//Perform the like update
			//Put in updates array
			//Compare timestamp w/ yours
			//Increase entropy vector
			//TODO
			//Multicast the update to all servers
		}
		else if(received_update.type == 0) {
			//Perform the new message update
			//Put in updates array
			//Increase entropy vector
			//TODO
			//Multicast the update to all servers
		}
		else if(received_update.type == 2) {
			//If all have been received, send out updates if you are the max
			//TODO: Deal with mid partition - ie) look for membership change
			entropy_received++;
			//TODO: update matrix based on new vector
			/*if(entropy_received = num in group) {
				Send_Merge_Updates();
			}*/
			//TODO: update local vector based on matrix
			Clear_Updates();
		}
			
	}else if( Is_membership_mess( service_type )) {
		
		//Check if it was an update from the server group
		if(target_groups[0] == SERVER_GROUP_NAME) {
			//If it was an addition, merge
			if(Is_caused_join_mess(servvice_type)) {
				//Send your anti-entropy vector
				SP_multicast(Mbox, AGREED_MESS, group, 1, MAX_MESSLEN, 
					(char *) entropy_matrix[machine_index]);
				entropy_received = 0;
			}
		}

		//Check if it was an update from a chatroom group
		//TODO - fix loop and if
		for (int i = 0; i < 0; i++) {
			if(target_groups[0] == "Test") {
				//Update your users list
				//Multicast the update to all servers if from client
				//TODO
			}
		}
	}
}

void Send_Merge_Updates()
{
	/** Send necessary updates **/
	for(int i = 0; i < NUM_SERVERS; i++) {
		if(isMax(entropy_matrix[i])) {
	 		for(int j = 0; j < NUM_SERVERS; j++) {
	 			//Send out the ones you have
	 		}
	 	}
	}
}

int Is_Max(int vector[])
{
	/** Checks if the server for the given anti-entropy vector is the server
	 *  with the most updates **/
	int server_val = vector[machine_index - 1];
	for(int i = 0; i < NUM_SERVERS; i++) {
		if(vector[i] > server_val)
			return 0;
		if(vector[i] == server_val) {
			if(i > machine_index)
				return 0;
		}
	}
	return 1;
}

int Min_Val(int vector[])
{
	/** Gets the minimum value in the array - for removing updates  **/
	int min = vector[0];
	for(int i = 1; i < NUM_SERVERS; i++)
		if(vector[i] < min)
			min = vector[i];

	return min;
}

void Send_All_Messages()
{
	//TODO
}

void Send_Recent_TwentyFive()
{
	//TODO
}

void Clear_Updates()
{
	//TODO
}
