/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Server Program
 *
 */

#include "linked_list.c"
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
int              num_groups;

int              entropy_matrix[NUM_SERVERS][NUM_SERVERS];
int              entropy_received = 0;

int              lamport_counter;

struct chatroom_node    head; //The main data structure
update_array            updates[NUM_SERVERS]; //Update struct for each server


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

	/** Initialize the udpate arrays **/
	for(int i = 0; i < NUM_SERVERS; i++) {
		updates[i].size = INITIAL_SIZE;
		updates[i].array = malloc(sizeof(update)*INITIAL_SIZE);
	}

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
	int16   mess_type;
	char    mess[1500];
	
	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups,
		target_groups, &mess_type, &endian_mismatch, sizeof(update), mess);
	
	if(Is_regular_mess( service_type )) {
		//Cast the message to an update
		received_update = *((update *) mess);

		/** Check if it is an unlike **/
		if(received_update.type == -1) {
			//Perform the unlike update on linked list
			//TODO - Need data structure

			//Put in updates array
			int origin = received_update.lamport.server_index;
			int element_count = updates[origin-1].element_count;
			updates[origin-1].array[element_count] = received_update;
			updates[origin-1].element_count++;
			updates[origin-1] = attempt_double(updates[origin-1]);

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector
			if(received_update.lamport.timestamp > entropy_matrix[machine_index-1][origin]) {
				entropy_matrix[machine_index-1][origin] = received_update.lamport.timestamp;
			}

			//Multicast the update to all servers if the update was from a client
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}
		}

		/** Check if it is a like  **/
		else if(received_update.type == 1) {
			//Perform the like update
			//TODO - Need data structure

			//Put in updates array
			int origin = received_update.lamport.server_index;
			int element_count = updates[origin-1].element_count;
			updates[origin-1].array[element_count] = received_update;;
			updates[origin-1].element_count++;
			updates[origin-1] = attempt_double(updates[origin-1]);

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector	
			if(received_update.lamport.timestamp > entropy_matrix[machine_index-1][origin]) {
				entropy_matrix[machine_index-1][origin] = received_update.lamport.timestamp;
			}

			//Multicast the update to all servers
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}
		}

		/** Check if it is a chat message **/
		else if(received_update.type == 0) {
			//Perform the new message update
			//TODO - Need data structure

			//Put in updates array
			int origin = received_update.lamport.server_index;
			int element_count = updates[origin-1].element_count;
			updates[origin-1].array[element_count] = received_update;;
			updates[origin-1].element_count++;
			updates[origin-1] = attempt_double(updates[origin-1]);

			//Compare timestamp with yours
			Compare_Lamport(received_update.lamport.timestamp);

			//Increase entropy vector
			if(received_update.lamport.timestamp > entropy_matrix[machine_index-1][origin]) {
				entropy_matrix[machine_index-1][origin] = received_update.lamport.timestamp;
			}

			//Multicast the update to all servers
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}
		}

		/** Check if it is an entropy vector for merging **/
		else if(received_update.type == 2) {
			//Update your matrix with the received message
			int origin = received_update.lamport.server_index;
			for(int i = 0; i < NUM_SERVERS; i++) {
				entropy_matrix[origin][i] = received_update.vector[i];
			}

			//TODO: Deal with mid partition - ie) look for membership change
			
			//If all have been received, send out updates if you are the max
			if(++entropy_received == num_groups) {
				Send_Merge_Updates();
			}
			
			//Update your local vector
			for(int i = 0; i < NUM_SERVERS; i++) {
				entropy_matrix[machine_index-1][i] = entropy_matrix[i][i];
			}

			Clear_Updates();
		}
			
	}else if( Is_membership_mess( service_type )) {
		
		/** Check if it was an update from the server group **/
		if(strcmp(target_groups[0], SERVER_GROUP_NAME) == 0) {
			//If it was an addition, merge
			if(Is_caused_join_mess(service_type)) {
				//Send your anti-entropy vector as an update
				//update entropy_update = malloc(sizeof(update));
				for(int i = 0; i < NUM_SERVERS; i++) {
					//entropy_update.vector[i] = entropy_matrix[machine_index-1][i];
				}
				//SP_multicast(Mbox, AGREED_MESS, group, 1, MAX_MESSLEN, 
				//	(char *) &update);
				entropy_received = 0;
			}

		/** The update came from a chatroom group **/
		}else{
			//Update your users list
			//TODO - Need data structure
				
			//Multicast the update to all servers
			//TODO - Create a "<user> joined" or "<user> left" update
			//TODO - Multicast the update
		}
	}
}

void Send_Merge_Updates()
{
	/** TODO: Should it be max in group? **/
	/** Send necessary updates **/
	for(int i = 0; i < NUM_SERVERS; i++) {
		if(Is_Max(entropy_matrix[i])) {
	 		for(int j = 0; j < NUM_SERVERS; j++) {
	 			//Send out the ones you have
				int current = updates[i].start;
				int s = updates[i].size;
				while(updates[i].array[current%s].lamport.timestamp < entropy_matrix[i][j]) {
					//Send the update to the machines which need it
					//TODO - Need to re-think this method

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
			if(i > machine_index-1)
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
	/** Sends all the messages it has from a particular chatroom to a specific client
	 * connected to the server and in that chatroom **/
	
	//TODO - Need data structure
}

void Send_Recent_TwentyFive()
{
	/** Sends the most recent 25 messages it has from a particular chatroom to a specific
	 * client connected to the server and in that chatroom **/

	//TODO - Need data structure
}

void Clear_Updates()
{
	/** Sets updates which are no longer needed to type = -2 in the udpates array**/
	int min = 0;
	int current;
	int size;
	for(int i = 0; i < NUM_SERVERS; i++) {
		min = Min_Val(entropy_matrix[i]);
		current = updates[i].start;
		size = updates[i].size;
		while(updates[i].array[current%size].lamport.timestamp < min) {
			updates[i].array[current%size].type = -2;
			current++;
		}
	}
}

void Compare_Lamport(int lamport)
{
	/** Compares a received lamport timestamp to the server's - if the received one is higer, 
	 * it sets the server's equal to it**/
	if(lamport > lamport_counter) {
		lamport_counter = lamport;
	}
}