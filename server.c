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
#include "update_array.c"

#define MAX_NAME 80
#define SERVER_GROUP_NAME "servers"
#define SPREAD_NAME "10030"
#define NUM_SERVERS 5
#define MAX_MEMBERS 100
#define MAX_MESSLEN 1400

/** Method Declarations **/
static void Handle_messages();
void Send_Merge_Updates();
int Is_Max(int[]);
int Min_Val(int[]);
void Send_All_Messages();
void Send_Recent_Twenty_Five();
void Clear_Updates();
void Compare_Lamport();

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

struct chatroom_node    head; //The main data structure
struct update_array     updates[NUM_SERVERS]; //Update struct for each server


int main(int argc, char *argv[])
{
	/** Set up updates arrays **/
	for(int i = 0; i < NUM_SERVERS; i++) {
		updates[i].start = 0;
		updates[i].size = 5;
		updates[i].element_count = 0;
	}

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

	/* Process and send messages using the Spread Event System */
	E_init();

	E_attach_fd(Mbox, READ_FD, Handle_messages, 0, NULL, HIGH_PRIORITY);

	E_handle_events();

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
	static char group[80];
	
	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups,
		target_groups, &mess_type, &endian_mismatch, sizeof(update), mess);
	
	if(Is_regular_mess( service_type )) {
		//received_update = (received_update) mess;
		if(received_update.type == -1) {
			//Perform the unlike update on linked list
			//TODO - Need data structure

			//Put in updates array
			int origin = received_update.lamport.server_index;
			update_array[origin-1].array[origin.element_count] = received_update;
			update_array[origin-1].element_count++;
			update_array[origin-1] = attempt_double[update_array[origin-1]];

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector
			if(received_update.lamport.timestamp > entropy_matrix[machine_index][origin]) {
				entropy_matrix[machine_index][origin] = received_udpate.lamport.timestamp;
			}

			//Multicast the update to all servers if the update was from a client
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS, server_group, 1, MAX_MESSLEN,
					(char *) received_update);
			}
		}
		else if(received_update.type == 1) {
			//Perform the like update
			//TODO - Need data structure

			//Put in updates array
			int origin = received_update.lamport.server_index;
			update_array[origin-1].array[origin.element_count] = received_update;;
			update_array[origin-1].element_count++:
			update_array[origin-1] = attempt_double[update_array[origin-1]];

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector	
			if(received_update.lamport.timestamp > entropy_matrix[machine_index][origin]) {
				entropy_matrix[machine_index][origin] = received_udpate.lamport.timestamp;
			}

			//Multicast the update to all servers
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS, server_group, 1, MAX_MESSLEN,
					(char *) received_update);
			}
		}
		else if(received_update.type == 0) {
			//Perform the new message update
			//TODO - Need data structure

			//Put in updates array
			int origin = received_update.lamport.server_index;
			update_array[origin-1].array[origin.element_count] = received_update;;
			update_array[origin-1].element_count++:
			update_array[origin-1] = attempt_double[update_array[origin-1]];

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector
			if(received_update.lamport.timestamp > entropy_matrix[machine_index][origin]) {
				entropy_matrix[machine_index][origin] = received_udpate.lamport.timestamp;
			}

			//Multicast the update to all servers
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS, server_group, 1, MAX_MESSLEN,
					(char *) received_update);
			}
		}
		else if(received_update.type == 2) {
			//If all have been received, send out updates if you are the max
			//TODO: Deal with mid partition - ie) look for membership change
			entropy_received++;
			//TODO: update matrix based on new vector
			/*if(entropy_received = num in group) {
				Send_Merge_Updates();
			}*/
			
			for(int i = 0; i < NUM_SERVERS; i++) {
				entropy_matrix[machine_index][i] = entropy_matrix[i][i];
			}

			Clear_Updates();
		}
			
	}else if( Is_membership_mess( service_type )) {
		
		//Check if it was an update from the server group
		if(strcmp(target_groups[0], SERVER_GROUP_NAME) == 0) {
			//If it was an addition, merge
			if(Is_caused_join_mess(service_type)) {
				//Send your anti-entropy vector
				SP_multicast(Mbox, AGREED_MESS, group, 1, MAX_MESSLEN, 
					(char *) entropy_matrix[machine_index]);
				entropy_received = 0;
			}
		}

		//Check if it was an update from a chatroom group
		//TODO - fix loop and if
		for (int i = 0; i < 0; i++) {
			if(strcmp(target_groups[0], "Test") == 0) {
				//Update your users list
				//Multicast the update to all servers if from client
				//TODO
			}
		}
	}
}

void Send_Merge_Updates()
{
	/** Should it be max in group? **/
	/** Send necessary updates **/
	for(int i = 0; i < NUM_SERVERS; i++) {
		if(Is_Max(entropy_matrix[i])) {
	 		for(int j = 0; j < NUM_SERVERS; j++) {
	 			//Send out the ones you have
				int current = updates[i].start;
				int s = updates[i].size;
				while(updates[i][current%s].lamport.timestamp < entropy_matrix[i][j]) {
					//Send the update to the machines which need it
					//send(updates[i][current%s]; -> Use private group
					current++;
				}
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
	//TODO - Need data structure
}

void Send_Recent_TwentyFive()
{
	//TODO - Need data structure
}

void Clear_Updates()
{
	/** Sets updates which are no longer needed to NULL in the udpates array**/
	int min = 0;
	int current;
	int size;
	for(int i = 0; i < NUM_SERVERS; i++) {
		min = Min_Val(entropy_matrix[i]);
		current = updates[i].start;
		size = updates[i].size;
		while(updates[i][current%size].lamport.timestamp < min) {
			updates[i][current%size] = NULL;
		}
	}
}

void Compare_Lamport(int lamport)
{
	if(lamport > lamport_counter) {
		lamport_counter = lamport;
	}
}
