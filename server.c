/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * High Availability Distributed Chat Service
 * Server Program
 *
 */

#include "data_structure.c"
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
void Send_All_Messages(char *, char *);
//void Send_Recent_TwentyFive(char *);
void Clear_Updates();
void Compare_Lamport();
void Send_Server_View();

/** Global Variables **/
static int       machine_index;

static char      Spread_name[MAX_NAME] = SPREAD_NAME;
static char      Private_group[MAX_NAME];
static char      server_group[MAX_NAME] = SERVER_GROUP_NAME;
static char      User[MAX_NAME] = "1";
char             current_group[NUM_SERVERS][MAX_GROUP_NAME];
int              joined_default = 0;

static mailbox   Mbox;
int              ret;
int              num_groups;
char             chatroom[MAX_NAME];
char             default_group[MAX_NAME];
char             individual_group[MAX_NAME];

int              entropy_matrix[NUM_SERVERS][NUM_SERVERS];
int              entropy_received = 0;

int              lamport_counter;

user_node        *to_change;

update_array            updates[NUM_SERVERS]; //Update struct for each server
//Note: The main data structure run in data_structure.c

int main(int argc, char *argv[])
{
	/** Check usage and set command line variables **/
	if(argc != 2) {
		printf("\nUsage: server <server_index>\n");
		exit(0);
	}
	machine_index = atoi(argv[1]);

	/** Set up timeout for Spread connection **/
	sp_time test_timeout;
	test_timeout.sec = 5;
	test_timeout.usec = 0;

	/** Set up the server name **/
	strcpy(individual_group, "server");
	char *machine_index_str;
	sprintf(machine_index_str, "%d", machine_index);
	strcat(individual_group, machine_index_str);

	/** Connect to the Spread client **/
	ret = SP_connect_timeout( Spread_name, individual_group, 0, 1, &Mbox, Private_group,
		test_timeout);
	if(ret != ACCEPT_SESSION)
	{
		SP_error(ret);
		exit(0);
	}

	/** Join the server group **/
	ret = SP_join(Mbox, server_group);
	if(ret < 0) SP_error(ret);
	printf("\nDebug> Server group joined: %s\n", server_group);

	/** Join the default client group **/
	strcpy(default_group, "default");
	strcat(default_group, machine_index_str);
	ret = SP_join(Mbox, default_group);
	if(ret < 0) SP_error(ret);

	/** Join the servers individual group **/
	ret = SP_join(Mbox, individual_group);
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
	char          target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	update        received_update;
	int           endian_mismatch;
	int           service_type;
	char          sender[MAX_GROUP_NAME];
	int16         mess_type;
	char          mess[MAX_MESSLEN];
	message_node  *changed_message;
	
    changed_message = malloc(sizeof(message_node));
	/** Receive a message **/
	ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS, &num_groups,
		target_groups, &mess_type, &endian_mismatch, MAX_MESSLEN, mess);
	
	if(ret < 0) {
		return;
	}
	
	if(Is_regular_mess( service_type )) {
		
		//Cast the message to an update
		received_update = *((update *) mess);

		/** Check if it is an unlike **/
		if(received_update.type == -1) {
			//Stamp the message
			received_update.lamport.timestamp = lamport_counter++;
			received_update.lamport.server_index = machine_index;
			
			//Perform the unlike update on linked list
			changed_message = unlike(received_update.user, received_update.liked_message_lamp, 
				target_groups[0]);

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
				SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}

			//Send the updated line to the clients in the chatroom connected to this server
			SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, received_update.chatroom, 1, MAX_MESSLEN, 
				(char *) changed_message);	
		}

		/** Check if it is a like **/
		else if(received_update.type == 1) {
			//Stamp the message
			received_update.lamport.timestamp = lamport_counter++;
			received_update.lamport.server_index = machine_index;

			//Perform the like update
			changed_message = like(received_update.user, received_update.lamport, 
			received_update.liked_message_lamp, received_update.chatroom);
            
            if (changed_message == NULL) {
                return;
            }

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
				SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}

			//Send the updated line to the clients in the chatroom connected to this server
			SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, received_update.chatroom, 1, MAX_MESSLEN,
				(char *) changed_message);
			printf("\nSENT BACK LIKED MESSAGE to %s\n", received_update.chatroom);
		}

		/** Check if it is a chat message **/
		else if(received_update.type == 0) {
			printf("\nDebug> Got an append message request\n");

			received_update.lamport.timestamp = lamport_counter++;
			
			//Perform the new message update
			changed_message = add_message(received_update.message, received_update.chatroom, 
				received_update.lamport);

			//Stamp the message
			changed_message->timestamp = lamport_counter;
			changed_message->server_index = machine_index;
			strcpy(changed_message->author, received_update.user);
	
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

			//Multicast the update to all servers
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1, MAX_MESSLEN,
					(char *) &received_update);
			}
			printf("\nSent message: %s\n", received_update.message);

			//Send the updated line to the clients in the chatroom connected to this server
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, received_update.chatroom, 1, MAX_MESSLEN,
				(char *) changed_message);
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

		/** Check if its a complete history request **/
		else if(received_update.type == 3) {
			//Send the complete history to the client which requested it
			printf("\n***************REQUEST FOR HISTORY***********\n");
			Send_All_Messages(received_update.chatroom, sender);
		}

		/** Check if its a network view request **/
		else if(received_update.type == 4) {
			//Send the network view to the client which requested it
			Send_Server_View();
		}

		/** Check if its a chatroom join message **/
		else if(received_update.type == 5) {
			printf("\nDebug> Got chatroom join message - sending private\n");
			printf("Debug> User joining: %s\n", received_update.user);
			printf("Debug> Room joining: %s\n", received_update.chatroom);
			ret = SP_join(Mbox, received_update.chatroom);

			/** Update the chatroom's users **/
			//Get the chatroom node for the update's chatroom
			chatroom_node *tmp;
			
			if(chatroom_head == NULL) {
				add_chatroom(received_update.chatroom);
			}

			tmp = chatroom_head;

			while(tmp->next != NULL) {
				if(strcmp(tmp->chatroom_name, received_update.chatroom)) {
					break;
				}
				tmp = tmp->next;
			}
			
			//Create the to_change node
			to_change = malloc(sizeof(user_node));
			to_change->user = received_update.user;
			to_change->connected_server = machine_index;
			
			//Add it to the data structure
			add_user(tmp, to_change);

			/** Send update to other server **/
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1,
					MAX_MESSLEN, (char *) &received_update);
			}
			
			/** Send update to the clients - wait for membership? **/
			message_node *mess_to_send;
			mess_to_send = malloc(sizeof(message_node));
			mess_to_send->timestamp = -1;
			strcpy(mess_to_send->author, received_update.user);
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, received_update.chatroom, 1,
				MAX_MESSLEN, (char *) mess_to_send);

			/** Send the new client who is in its chatroom **/
			chatroom_node *tmp_room = chatroom_head;
			//Get the chatroom node
			while(tmp_room->next != NULL) {
				if(strcmp(tmp_room->chatroom_name, received_update.chatroom) == 0) {
					break;
				}
				tmp_room = tmp_room->next;
			}
			//Send the user updates in the server's data structure to the new client
			user_node *tmp_user = tmp_room->user_list_head;
			strcpy(mess_to_send->author, tmp_user->user);
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, sender, 1, MAX_MESSLEN, 
				(char *) mess_to_send);
			printf("\nJoin Message sent back with user %s\n", mess_to_send->author);
			while(tmp_user->next != NULL) {
				strcpy(mess_to_send->author, tmp_user->user);
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, sender, 1, MAX_MESSLEN, 
					(char *) mess_to_send);
				printf("Join Message sent back with user %s\n", mess_to_send->author);
				tmp_user = tmp_user->next;
			}
		}

		/** Check if its a chatroom leave message **/
		else if(received_update.type == 6) {
			printf("\nDebug> Got a chatroom leave message\n");

			/** Update the chatroom's users **/
			//Get the chatroom node for the update's chatroom
			chatroom_node *tmp;
			
			if(chatroom_head == NULL) {
				add_chatroom(received_update.chatroom);
			}

			tmp = chatroom_head;

			while(tmp->next != NULL) {
				if(strcmp(tmp->chatroom_name, received_update.chatroom)) {
					break;
				}
				tmp = tmp->next;
			}
			
			//Create the to_change node
			to_change->user = received_update.user;
			to_change->connected_server = machine_index;
			
			//Remove it from the data structure
			remove_user(tmp, to_change);
			
			/** Send update to other servers **/
			if(strcmp(target_groups[0], SERVER_GROUP_NAME) != 0) {
				SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, server_group, 1,
					MAX_MESSLEN, (char *) &received_update);
			}
			
			/** Send update to clients **/
			message_node *mess_to_send;
			mess_to_send = malloc(sizeof(message_node));
			mess_to_send->timestamp = -2;
			strcpy(mess_to_send->author, received_update.user);
			SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, received_update.chatroom, 1,
				MAX_MESSLEN, (char *) mess_to_send);

			printf("\nSent leave of %s\n", mess_to_send->author);
			printf("\nSent left message to %s\n", received_update.chatroom);
		}
			
	}else if( Is_membership_mess( service_type )) {
		
		/** Check if it was an update from the server group **/
		if(strcmp(sender, SERVER_GROUP_NAME) == 0) {
			printf("\nDebug> Membership message from the server group");
			printf("\nDebug> Message from: %s\n", sender);

			/** Set the new view of chat servers in the current server's newtwork component**/
			for(int i = 0; i < NUM_SERVERS; i++) {
				for(int j = 0; j < MAX_GROUP_NAME; j++) { 
					current_group[i][j] = target_groups[i][j];
				}
			}

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
			printf("\nDebug> Membership message from a chatroom group");
			printf("\nDebug> Message from: %s\n", sender);	
			
			/** Send the new client the recent 25 messages **/
			if(Is_caused_join_mess(service_type)) {
				
				/** Send the new client the recent 25 messages **/
				printf("\nSender: %s\n", sender);
				printf("Def: %s\n", default_group);
				printf("Ind: %s\n", individual_group);
				printf("\nTrying to send 25\n");
				if(strcmp(sender, default_group) != 0) {
					//if(strcmp(sender, individual_group != 0)) {
						printf("\nCalling send 25      - -******\n");
						Send_Recent_TwentyFive(sender);
					//}
				}
			}
		}
	}
}

void Send_Merge_Updates()
{
	/** TODO: Should it be max in group? **/
	//TODO: AFTER WORKING
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

void Send_All_Messages(char *room_name, char *client_private_group)
{
	/** Sends all the messages it has from a particular chatroom to a specific client
	 * connected to the server and in that chatroom **/
	
	struct chatroom_node * curr_room = chatroom_head;
	struct message_node * curr_mess;
	struct message_node * tmp;
	struct message_node * tmp2; //goes to 25th to last
	struct message_node * tmp3; //goes to last

	/** Look for correct chatroom **/
	while(strcmp(curr_room->chatroom_name, room_name) != 0) {
		
		//Deal with case if the chatroom is not in the data structure
		if(curr_room->next == NULL) {
			return;
		}
		curr_room = curr_room->next;
	}
	
	/** Iterate tmp3 25 spaces ahead **/
	tmp3 = curr_room->mess_head;
	int end = 0;
	while(curr_room != NULL && end < 25) {
		tmp3 = tmp3->next;
		end++;
	}

	/** Iterate tmp3 and tmp2 until tmp3 is null **/
	while(tmp3 != NULL) {
		tmp2 = tmp2->next;
		tmp3 = tmp3->next;
	}

	/** Iterate through and print out until at tmp2**/
	curr_mess = curr_room->mess_head;
	while(curr_mess->next != NULL) {
		//Multicast the message to the client which requested the messages
		//TODO: Set next to null
		tmp = curr_mess->next;
		curr_mess->next = NULL;
		SP_multicast(Mbox, AGREED_MESS | SELF_DISCARD, client_private_group, 1, MAX_MESSLEN,
			(char *) &curr_mess);
		curr_mess = tmp;
	}

	/** Send message noting that sending is gone **/
	curr_mess->timestamp = -3;
	SP_multicast(Mbox, AGREED_MESS, client_private_group, 1, MAX_MESSLEN,
		(char *) curr_mess);
}


void Send_Recent_TwentyFive(char *room_name)
{
	/** Sends the most recent 25 messages it has from a particular chatroom to a specific
	 * client connected to the server and in that chatroom **/

	struct chatroom_node * curr_room = chatroom_head;
	struct message_node * curr_mess;
	struct message_node * curr_mess_end;

	if(curr_room == NULL) {
		return;
	}

	/** Look for the correct chatroom **/
	printf("\nCurrent Room: %s\n", curr_room->chatroom_name);
	printf("\nRoom name: %s\n", room_name);
	while(strcmp(curr_room->chatroom_name, room_name) != 0) {
		if(curr_room->next == NULL) {
			return;
		}
		curr_room = curr_room->next;
	}

	curr_mess = curr_room->mess_head;
	curr_mess_end = curr_room->mess_head;

	/** Move curr_mess_end 25 spaces ahead of curr_mess **/
	int end = 0;
	while(curr_mess_end != NULL && end < 25) {
		curr_mess_end = curr_mess_end->next;
		end++;
		printf("\nMoving End pointer\n");
	}

	/** Move both pointers so we have one at 25th to last and one at last **/
	while(curr_mess_end != NULL) {
		curr_mess_end = curr_mess_end->next;
		curr_mess = curr_mess->next;
	}

	/** Send the last 25 messages **/
	while(curr_mess != NULL) {
		SP_multicast(Mbox, AGREED_MESS|SELF_DISCARD, room_name, 1, MAX_MESSLEN, 
			(char *) curr_mess);
		printf("\nSending 25: %s------------>*\n", curr_mess->message);
		curr_mess = curr_mess->next;
	}
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

void Send_Server_View()
{
	/** Sends a view of the chat servers in the current server's network to the client 
	 *  who requested the view**/
	/*printf("\nServers in the current server's network segment:\n");
	for(int i = 0; i < NUM_SERVERS; i++) {
		printf("%d) %s", i, current_group[i]);
	}*/
	//TODO: SEND ---- DO NOT PRINT
}
